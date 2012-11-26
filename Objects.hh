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

#ifndef OBJECTS_HH
#define OBJECTS_HH

#include <algorithm>

#include "ObjectBases.hh"

class Node : public ObjectBase, public LonLat, public TagContainer {
public:
	Node(osmint_t id, int lat, int lon) : ObjectBase(id), LonLat(lon, lat) {
	}

	void Dump(std::ostream& stream = std::cout) const {
		stream << "  <node ";
		ObjectBase::Dump(stream);
		stream << " ";
		LonLat::Dump(stream);
		if (TagContainer::HasTags()) {
			stream << ">" << std::endl;
			TagContainer::Dump(stream);
			stream << "  </node>" << std::endl;
		} else {
			stream << "/>" << std::endl;
		}
	}
};

class Way : public ObjectBase, public TagContainer {
private:
	typedef std::vector<osmid_t> NodeVector;

private:
	NodeVector nodes_;

public:
	Way(osmint_t id) : ObjectBase(id) {
	}

	void AddNode(osmid_t id) {
		nodes_.push_back(id);
	}

	void ClearNodes() {
		nodes_.clear();
	}

	int RemoveNode(osmid_t id) {
		NodeVector temp;
		temp.reserve(nodes_.size());

		int nremoved = 0;
		for (NodeVector::const_iterator node = nodes_.begin(); node != nodes_.end(); ++node)
			if (*node == id)
				++nremoved;
			else
				temp.push_back(*node);

		nodes_.swap(temp);

		return nremoved;
	}

	void RemoveNodeAt(int pos) {
		nodes_.erase(nodes_.begin() + pos);
	}

	void RemoveNodesAt(int pos, int count) {
		nodes_.erase(nodes_.begin() + pos, nodes_.begin() + pos + count);
	}

	bool HasNodes() const {
		return !nodes_.empty();
	}

	bool HasNode(osmid_t id) const {
		return std::find(nodes_.begin(), nodes_.end(), id) != nodes_.end();
	}

	int GetNodesCount() const {
		return nodes_.size();
	}

	bool IsClosed() const {
		return !nodes_.empty() && nodes_.front() == nodes_.back();
	}

	void Reverse() {
		std::reverse(nodes_.begin(), nodes_.end());
	}

	osmid_t& NodeAt(int pos) {
		return nodes_.at(pos);
	}

	const osmid_t& NodeAt(int pos) const {
		return nodes_.at(pos);
	}

	void CloseWay() {
		if (!nodes_.empty() && nodes_.front() != nodes_.back())
			nodes_.push_back(nodes_.front());
	}

	void Dump(std::ostream& stream = std::cout) const {
		stream << "  <way ";
		ObjectBase::Dump(stream);
		stream << ">" << std::endl;
		for (NodeVector::const_iterator node = nodes_.begin(); node != nodes_.end(); ++node)
			stream << "    <nd ref=\"" << *node << "\"/>" << std::endl;
		TagContainer::Dump(stream);
		stream << "  </way>" << std::endl;
	}
};

class Relation : public ObjectBase, public TagContainer  {
public:
	enum MemberType {
		NODE,
		WAY,
		RELATION
	};

private:
	struct Member {
		MemberType type;
		osmid_t id;
		std::string role;

		Member(MemberType t, osmid_t i, const std::string& r) : type(t), id(i), role(r) {
		}
	};

	typedef std::vector<Member> MemberVector;

private:
	MemberVector members_;

public:
	Relation(osmint_t id) : ObjectBase(id) {
	}

	void AddMember(MemberType type, osmid_t id, const std::string role) {
		members_.push_back(Member(type, id, role));
	}

	void ClearMembers() {
		members_.clear();
	}

	int RemoveMember(MemberType type, osmid_t id) {
		MemberVector temp;
		temp.reserve(members_.size());

		int nremoved = 0;
		for (MemberVector::const_iterator member = members_.begin(); member != members_.end(); ++member)
			if (member->type == type && member->id == id)
				++nremoved;
			else
				temp.push_back(*member);

		members_.swap(temp);

		return nremoved;
	}

	void RemoveMemberAt(int pos) {
		members_.erase(members_.begin() + pos);
	}

	void RemoveMembersAt(int pos, int count) {
		members_.erase(members_.begin() + pos, members_.begin() + pos + count);
	}

	bool HasMembers() const {
		return members_.empty();
	}

	bool HasMember(MemberType type, osmid_t id) const {
		for (MemberVector::const_iterator member = members_.begin(); member != members_.end(); ++member)
			if (member->type == type && member->id == id)
				return true;

		return false;
	}

	int GetMembersCount() const {
		return members_.size();
	}

	void Reverse() {
		std::reverse(members_.begin(), members_.end());
	}

	Member& MemberAt(int pos) {
		return members_.at(pos);
	}

	const Member& MemberAt(int pos) const {
		return members_.at(pos);
	}

	void Dump(std::ostream& stream = std::cout) const {
		stream << "  <relation ";
		ObjectBase::Dump(stream);
		stream << ">" << std::endl;
		for (MemberVector::const_iterator member = members_.begin(); member != members_.end(); ++member) {
			stream << "    <member type=\"";
			switch (member->type) {
			case NODE: stream << "node"; break;
			case WAY: stream << "way"; break;
			case RELATION: stream << "relation"; break;
			}
			stream << "\" ref=\"" << member->id << "\" role=\"" << XMLEncodeAttr(member->role) << "\"/>" << std::endl;
		}
		TagContainer::Dump(stream);
		stream << "  </relation>" << std::endl;
	}
};

#endif
