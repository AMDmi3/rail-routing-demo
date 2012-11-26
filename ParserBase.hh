/*
 * Copyright (C) 2012 Dmitry Marakasov
 *
 * This file is part of rail routing demo.
 *
 * rail routing demo is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * rail routing demo is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with rail routing demo.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PARSERBASE_HH
#define PARSERBASE_HH

#include <expat.h>
#include <fcntl.h>
#include <unistd.h>

#include <vector>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cstring>

#include "Objects.hh"

class ParsingException : public std::runtime_error {
public:
	ParsingException(const std::string& what) : std::runtime_error(what) {
	}
};

template<int I>
static int ParseInt(const char* str) {
	int value = 0;
	int fracdig = 0;
	int haddot = 0;
	bool neg = false;
	const char* cur = str;

	if (*cur == '-') {
		neg = true;
		cur++;
	}

	for (; *cur != '\0'; ++cur) {
		if (*cur >= '0' && *cur <= '9') {
			value = value * 10 + *cur - '0';
			if (haddot && ++fracdig == I)
				break;
		} else if (*cur == '.') {
			haddot++;
		} else {
			throw ParsingException("bad coordinate format (unexpected symbol)");
		}
	}

	if (haddot > 1)
		throw ParsingException("bad coordinate format (multiple dots)");

	for (; fracdig < I; ++fracdig)
		value *= 10;

	return neg ? -value : value;
}

template <class Parser>
class ParserBase {
protected:
	typedef void(Parser::*ProcessNodeFn)(Node& node);
	typedef void(Parser::*ProcessWayFn)(Way& way);
	typedef void(Parser::*ProcessRelationFn)(Relation& relation);
	typedef void(Parser::*SimplePassFn)();

private:
	struct Pass {
		ProcessNodeFn node;
		ProcessWayFn way;
		ProcessRelationFn relation;
		SimplePassFn pass;
		bool dumps_data;
		std::string name;

		Pass(ProcessNodeFn n, ProcessWayFn w, ProcessRelationFn r, SimplePassFn p, bool d, const std::string& nm) : node(n), way(w), relation(r), pass(p), dumps_data(d), name(nm) {
		}
	};

	struct State {
		TagType type;
		std::auto_ptr<Node> node;
		std::auto_ptr<Way> way;
		std::auto_ptr<Relation> relation;

		const Pass& pass;
		Parser& parser;

		State(const Pass& p, Parser& r) : pass(p), parser(r) {
		}
	};

	typedef std::vector<Pass> PassVector;

private:
	PassVector passes_;

	bool dump_opened_;

private:
	void DumpOpen() {
		std::cout << "<?xml version='1.0' encoding='UTF-8'?>" << std::endl;
		std::cout << "<osm version=\"0.6\" generator=\"mposm\">" << std::endl;
		//std::cout << "<osm version=\"0.6\" generator=\"Osmosis 0.38\">" << std::endl;
		//std::cout << "  <bound box=\"40.98097,-180.00000,85.46578,180.00000\" origin=\"http://www.openstreetmap.org/api/0.6\"/>" << std::endl;
	}

	void DumpClose() {
		std::cout << "</osm>" << std::endl;
	}

	void DoPass(const Pass& pass, const char* filename) {
		int f = 0;
		XML_Parser parser = NULL;

		/* if filename = "-", work with stdin */
		if (strcmp(filename, "-") != 0 && (f = open(filename, O_RDONLY)) == -1)
			throw std::runtime_error("cannot open input file");

		/* Create and setup parser */
		if ((parser = XML_ParserCreate(NULL)) == NULL) {
			close(f);
			throw std::runtime_error("cannot create XML parser");
		}

		State state(pass, *static_cast<Parser*>(this));

		XML_SetElementHandler(parser, StartElement, EndElement);

		XML_SetUserData(parser, &state);

		/* Parse file */
		try {
			char buf[65536];
			ssize_t len;
			do {
				if ((len = read(f, buf, sizeof(buf))) < 0)
					throw std::runtime_error("input read error");
				if (XML_Parse(parser, buf, len, len == 0) == XML_STATUS_ERROR)
					throw ParsingException(XML_ErrorString(XML_GetErrorCode(parser)));
			} while (len != 0);
		} catch (ParsingException &e) {
			std::stringstream ss;
			ss << "error parsing input: " << e.what() << " at line " << XML_GetCurrentLineNumber(parser) << " pos " << XML_GetCurrentColumnNumber(parser);
			close(f);
			XML_ParserFree(parser);
			throw ParsingException(ss.str());;
		} catch (...) {
			close(f);
			XML_ParserFree(parser);
			throw;
		}

		XML_ParserFree(parser);
		close(f);
	}

	static void StartElement(void* userData, const char* name, const char** atts) {
		State& state = *static_cast<State*>(userData);

		if (state.type != NOTAG && strcmp(name, "tag") == 0) {
			const char* key = NULL;
			const char* value = NULL;
			for (const char** att = atts; *att; att += 2) {
				if ((*att)[0] == 'k') key = *(att + 1);
				else if ((*att)[0] == 'v') value = *(att + 1);
			}
			if (key == NULL || value == NULL)
				throw ParsingException("bad tag");
			switch (state.type) {
			case NODE:
				if (state.node.get())
					state.node->AddTag(key, value);
				break;
			case WAY:
				if (state.way.get())
					state.way->AddTag(key, value);
				break;
			case RELATION:
				if (state.relation.get())
					state.relation->AddTag(key, value);
				break;
			default:
				break;
			}
		} else if (state.type == WAY && strcmp(name, "nd") == 0 && state.way.get()) {
			const char* ref = NULL;
			for (const char** att = atts; *att; att += 2) {
				if (strcmp(*att, "ref") == 0) ref = *(att + 1);
			}

			if (ref == NULL)
				throw ParsingException("bad node reference");

			state.way->AddNode(strtoul(ref, NULL, 10));
		} else if (state.type == RELATION && strcmp(name, "member") == 0 && state.relation.get()) {
			const char* type = NULL;
			const char* ref = NULL;
			const char* role = NULL;
			for (const char** att = atts; *att; att += 2) {
				if (strcmp(*att, "type") == 0) type = *(att + 1);
				else if (strcmp(*att, "ref") == 0) ref = *(att + 1);
				else if (strcmp(*att, "role") == 0) role = *(att + 1);
			}

			if (type == NULL || ref == NULL || role == NULL)
				throw ParsingException("bad relation member");

			Relation::MemberType t;
			if (strcmp(type, "node") == 0)
				t = Relation::NODE;
			else if (strcmp(type, "way") == 0)
				t = Relation::WAY;
			else if (strcmp(type, "relation") == 0)
				t = Relation::RELATION;
			else
				throw ParsingException("bad relation member");

			state.relation->AddMember(t, strtoul(ref, NULL, 10), role);
		}

	   	if (strcmp(name, "node") == 0)
			state.type = NODE;
		else if (strcmp(name, "way") == 0)
			state.type = WAY;
		else if (strcmp(name, "relation") == 0)
			state.type = RELATION;
		else return;

		const char* id = NULL;
		const char* lat = NULL;
		const char* lon = NULL;

		for (const char** att = atts; *att; att += 2) {
			if (strcmp(*att, "id") == 0) id = *(att + 1);
			else if (strcmp(*att, "lat") == 0) lat = *(att + 1);
			else if (strcmp(*att, "lon") == 0) lon = *(att + 1);
		}

		if (id == NULL)
			throw ParsingException("bad id");

		if (state.type == NODE && (lat == NULL || lon == NULL))
			throw ParsingException("bad node");

		switch (state.type) {
		case NODE:
			state.node.reset(new Node(
					strtol/*l*/(id, NULL, 10),
					ParseInt<7>(lat),
					ParseInt<7>(lon)
				));
			break;
		case WAY:
			state.way.reset(new Way(
					strtol/*l*/(id, NULL, 10)
				));
			break;
		case RELATION:
			state.relation.reset(new Relation(
					strtol/*l*/(id, NULL, 10)
				));
			break;
		default:
			break;
		}
	}

	static void EndElement(void* userData, const char* name) {
		State& state = *static_cast<State*>(userData);
		const Pass& pass = state.pass;
		Parser& parser = state.parser;

		if (strcmp(name, "node") == 0 && state.type == NODE && pass.node && state.node.get())
			(static_cast<Parser&>(parser).*(pass.node))(*state.node.get());
		if (strcmp(name, "way") == 0 && state.type == WAY && pass.way && state.way.get())
			(static_cast<Parser&>(parser).*(pass.way))(*state.way.get());
		if (strcmp(name, "relation") == 0 && state.type == RELATION && pass.relation && state.relation.get())
			(static_cast<Parser&>(parser).*(pass.relation))(*state.relation.get());

		if (strcmp(name, "node") == 0 || strcmp(name, "way") == 0 || strcmp(name, "relation") == 0) {
			state.node.reset(NULL);
			state.way.reset(NULL);
			state.relation.reset(NULL);

			state.type = NOTAG;
		}
	}

protected:
	void AddPass(ProcessNodeFn node, bool dumps_data = false, const std::string name = "") {
		passes_.push_back(Pass(node, NULL, NULL, NULL, dumps_data, name));
	}

	void AddPass(ProcessWayFn way, bool dumps_data = false, const std::string name = "") {
		passes_.push_back(Pass(NULL, way, NULL, NULL, dumps_data, name));
	}

	void AddPass(ProcessRelationFn relation, bool dumps_data = false, const std::string name = "") {
		passes_.push_back(Pass(NULL, NULL, relation, NULL, dumps_data, name));
	}

	void AddPass(SimplePassFn pass, bool dumps_data = false, const std::string name = "") {
		passes_.push_back(Pass(NULL, NULL, NULL, pass, dumps_data, name));
	}

	void AddPass(ProcessNodeFn node, ProcessWayFn way, ProcessRelationFn relation, bool dumps_data = false, const std::string name = "") {
		passes_.push_back(Pass(node, way, relation, NULL, dumps_data, name));
	}

	void AddPass(ProcessNodeFn node, ProcessWayFn way, ProcessRelationFn relation, SimplePassFn pass, bool dumps_data = false, const std::string name = "") {
		passes_.push_back(Pass(node, way, relation, pass, dumps_data, name));
	}

public:
	ParserBase() : dump_opened_(false) {
	}

	void Parse(const char* filename) {
		int npass = 1;
		for(typename PassVector::const_iterator pass = passes_.begin(); pass != passes_.end(); ++pass) {
			if (pass->name.empty())
				std::cerr << "Pass " << npass++ << " of " << passes_.size() << std::endl;
			else
				std::cerr << "Pass " << npass++ << " of " << passes_.size() << ": " << pass->name << std::endl;

			if (pass->dumps_data && !dump_opened_) {
				DumpOpen();
				dump_opened_ = true;
			}
			if (pass->node || pass->way || pass->relation)
				DoPass(*pass, filename);
			if (pass->pass)
				(static_cast<Parser*>(this)->*(pass->pass))();
		}

		if (dump_opened_)
			DumpClose();
	}
};

#endif
