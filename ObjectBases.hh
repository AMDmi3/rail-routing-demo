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

#ifndef OBJECTBASES_HH
#define OBJECTBASES_HH

#include "osmtypes.h"

#include <map>
#include <string>
#include <iostream>
#include <iomanip>

enum Action {
	NOACTION = 0,
	CREATE,
	MODIFY,
	DELETE,
};

enum TagType {
	NOTAG = 0,
	NODE,
	WAY,
	RELATION,
};

static std::string XMLEncodeAttr(const std::string &str) {
	std::string temp;
	//temp.reserve(str.size);
	for (std::string::const_iterator i = str.begin(); i != str.end(); ++i) {
		switch (*i) {
		case '&':  temp += "&amp;"; break;
		case '<':  temp += "&lt;"; break;
		case '>':  temp += "&gt;"; break;
		case '"':  temp += "&quot;"; break;
		case '\'': temp += "&apos;"; break;
		case 9:    temp += "&#x9;"; break;
		case 10:   temp += "&#xA;"; break;
		case 13:   temp += "&#xD;"; break;
		default:   temp += *i; break;
		}
	}

	return temp;
}

class ObjectBase {
public:
	osmid_t id_;

	Action action_;

public:
	ObjectBase(osmid_t id) : id_(id) {
	}

	osmid_t GetId() const {
		return id_;
	}

	void SetModify() {
		action_ = MODIFY;
	}

	void SetDelete() {
		action_ = DELETE;
	}

	void ResetAction() {
		action_ = NOACTION;
	}

	bool IsModified() const {
		return action_ == MODIFY;
	}

	bool IsDeleted() const {
		return action_ == DELETE;
	}

	void Dump(std::ostream& stream = std::cout) const {
		stream << "id=\"" << id_ << "\"";

		if (action_ == MODIFY)
			stream << " action=\"modify\"";
		else if (action_ == DELETE)
			stream << " action=\"delete\"";
		else if (action_ == CREATE)
			stream << " action=\"create\"";
	}
};

class LonLat {
private:
	static const int shift_ = 10000000;

private:
	osmint_t lon_;
	osmint_t lat_;

private:
	void DumpCoord(std::ostream& stream, osmint_t coord) const {
		int val = coord;
		bool neg = false;
		if (val < 0) {
			val = -val;
			neg = true;
		}
		int hi = val / shift_;
		int lo = val - hi * shift_;

		int w = 0;
		if (lo == 0) {
			w = 0;
		} else {
			while (lo % 10 == 0) {
				lo /= 10;
				++w;
			}
		}

		if (lo)
			stream << (neg ? "-" : "") << hi << "." << std::setw(7-w) << std::setfill('0') << lo;
		else
			stream << (neg ? "-" : "") << hi;
	}

public:
	LonLat(osmint_t lon, osmint_t lat)
		: lon_(lon),
		  lat_(lat) {
	}

	osmint_t GetLonI() const {
		return lon_;
	}

	osmint_t GetLatI() const {
		return lat_;
	}

	float GetLonF() const {
		return (float)lon_ / (float)shift_;
	}

	float GetLatF() const {
		return (float)lat_ / (float)shift_;
	}

	double GetLonD() const {
		return (double)lon_ / (double)shift_;
	}

	double GetLatD() const {
		return (double)lat_ / (double)shift_;
	}

	void SetLonI(osmint_t lon) {
		lon_ = lon;
	}

	void SetLatI(osmint_t lat) {
		lat_ = lat;
	}

	void Dump(std::ostream& stream = std::cout) const {
		stream << "lat=\"";
		DumpCoord(stream, lat_);
		stream << "\" lon=\"";
		DumpCoord(stream, lon_);
		stream << "\"";
	}

	bool IsValid() const {
		return true;
	}
};

class TagContainer {
public:
	typedef std::map<std::string, std::string> TagMap;

private:
	TagMap tags_;

public:
	TagContainer() {
	}

	bool AddTag(const std::string& key, const std::string& value) {
		std::pair<TagMap::iterator, bool> res = tags_.insert(std::make_pair(key, value));
		return res.second;
	}

	bool ChangeTag(const std::string& key, const std::string& value) {
		std::pair<TagMap::iterator, bool> res = tags_.insert(std::make_pair(key, value));
		if (!res.second)
			return false;
		res.first->second = value;
		return true;
	}

	bool SetTag(const std::string& key, const std::string& value) {
		std::pair<TagMap::iterator, bool> res = tags_.insert(std::make_pair(key, value));
		if (!res.second)
			res.first->second = value;
		return res.second;
	}

	bool GetTag(const std::string& key, std::string& value) const {
		TagMap::const_iterator i = tags_.find(key);
		if (i == tags_.end())
			return false;
		value = i->second;
		return true;
	}

	std::string GetTag(const std::string& key) const {
		TagMap::const_iterator i = tags_.find(key);
		if (i == tags_.end())
			return "";
		return i->second;
	}

	std::string GetKeyAt(int pos) const {
		int n = 0;
		for (TagMap::const_iterator i = tags_.begin(); i != tags_.end(); ++i, ++n)
			if (n == pos)
				return i->first;

		return "";
	}

	std::string GetValAt(int pos) const {
		int n = 0;
		for (TagMap::const_iterator i = tags_.begin(); i != tags_.end(); ++i, ++n)
			if (n == pos)
				return i->second;

		return "";
	}

	bool RemoveTag(const std::string& key) {
		return tags_.erase(key);
	}

	bool HasTag(const std::string& key) const {
		return tags_.find(key) != tags_.end();
	}

	bool HasTags() const {
		return !tags_.empty();
	}

	bool IsTag(const std::string& key, const std::string& value) const {
		TagMap::const_iterator it = tags_.find(key);
		if (it == tags_.end())
			return false;
		return it->second == value;
	}

	int GetTagsCount() const {
		return tags_.size();
	}

	void Dump(std::ostream& stream = std::cout) const {
		for (TagMap::const_iterator tag = tags_.begin(); tag != tags_.end(); ++tag)
			stream << "    <tag k=\"" << XMLEncodeAttr(tag->first) << "\" v=\"" << XMLEncodeAttr(tag->second) << "\"/>" << std::endl;
	}

	const TagMap& GetTagMap() const {
		return tags_;
	}
};

#endif
