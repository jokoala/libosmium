#ifndef OSMIUM_OSM_OBJECT_HPP
#define OSMIUM_OSM_OBJECT_HPP

/*

This file is part of Osmium (http://osmcode.org/osmium).

Copyright 2013 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <cstdint>

#include <boost/operators.hpp>

#include <osmium/osm/types.hpp>
#include <osmium/osm/location.hpp>
#include <osmium/osm/tag.hpp>
#include <osmium/utils/timestamp.hpp>

namespace osmium {

    /**
     * OSM Object (Node, Way, or Relation).
     */
    class Object : public osmium::memory::Item, boost::less_than_comparable<Object> {

        object_id_type      m_id;
        object_version_type m_deleted_and_version;
        timestamp_type      m_timestamp;
        user_id_type        m_uid;
        changeset_id_type   m_changeset;

        static constexpr int deleted_bit = sizeof(object_version_type) * 8 - 1;
        static constexpr object_version_type deleted_bit_mask = 1 << deleted_bit;

        size_t sizeof_object() const {
            return sizeof(Object) + (type() == item_type::node ? sizeof(osmium::Location) : 0);
        }

        char* user_position() {
            return self() + sizeof_object();
        }

        const char* user_position() const {
            return self() + sizeof_object();
        }

        size_t user_length() const {
            return *reinterpret_cast<const size_t*>(user_position());
        }

        char* subitems_position() {
            return user_position() + sizeof(size_t) + osmium::memory::padded_length(user_length());
        }

        const char* subitems_position() const {
            return user_position() + sizeof(size_t) + osmium::memory::padded_length(user_length());
        }

    protected:

        template <class T>
        T& subitem_of_type() {
            for (iterator it = begin(); it != end(); ++it) {
                if (it->type() == item_traits<typename std::remove_const<T>::type>::itemtype) {
                    return reinterpret_cast<T&>(*it);
                }
            }

            static T subitem;
            return subitem;
        }

        template <class T>
        const T& subitem_of_type() const {
            for (const_iterator it = cbegin(); it != cend(); ++it) {
                if (it->type() == item_traits<typename std::remove_const<T>::type>::itemtype) {
                    return reinterpret_cast<const T&>(*it);
                }
            }

            static T subitem;
            return subitem;
        }

    public:

        Object() :
            m_id(0),
            m_deleted_and_version(0),
            m_timestamp(0),
            m_uid(0),
            m_changeset(0) {
        }

        object_id_type id() const {
            return m_id;
        }

        Object& id(object_id_type id) {
            m_id = id;
            return *this;
        }

        Object& id(const char* id) {
            return this->id(osmium::string_to_object_id(id));
        }

        object_version_type version() const {
            return m_deleted_and_version & (~deleted_bit_mask);
        }

        bool deleted() const {
            return m_deleted_and_version & deleted_bit_mask;
        }

        bool visible() const {
            return !deleted();
        }

        Object& version(object_version_type version) {
            m_deleted_and_version = (m_deleted_and_version & deleted_bit_mask) | (version & (~deleted_bit_mask));
            return *this;
        }

        Object& version(const char* version) {
            return this->version(string_to_object_version(version));
        }

        Object& deleted(bool deleted) {
            m_deleted_and_version = (m_deleted_and_version & (~deleted_bit_mask)) | ((deleted ? 1 : 0) << deleted_bit);
            return *this;
        }

        Object& visible(bool visible) {
            m_deleted_and_version = (m_deleted_and_version & (~deleted_bit_mask)) | ((visible ? 0 : 1) << deleted_bit);
            return *this;
        }

        Object& visible(const char* visible) {
            if (!strcmp("true", visible)) {
                this->visible(true);
            } else if (!strcmp("false", visible)) {
                this->visible(false);
            } else {
                throw std::runtime_error("unknown value for visible attribute");
            }
            return *this;
        }

        changeset_id_type changeset() const {
            return m_changeset;
        }

        Object& changeset(changeset_id_type changeset) {
            m_changeset = changeset;
            return *this;
        }

        Object& changeset(const char* changeset) {
            return this->changeset(string_to_changeset_id(changeset));
        }

        user_id_type uid() const {
            return m_uid;
        }

        Object& uid(user_id_type uid) {
            m_uid = uid;
            return *this;
        }

        Object& uid_from_signed(int32_t uid) {
            m_uid = uid < 0 ? 0 : uid;
            return *this;
        }

        Object& uid(const char* uid) {
            return this->uid(string_to_user_id(uid));
        }

        bool user_is_anonymous() const {
            return m_uid == 0;
        }

        time_t timestamp() const {
            return m_timestamp;
        }

        /**
         * Set the timestamp when this object last changed.
         * @param timestamp Time in seconds since epoch.
         * @return Reference to object to make calls chainable.
         */
        Object& timestamp(time_t timestamp) {
            m_timestamp = timestamp;
            return *this;
        }

        Object& timestamp(const char* timestamp) {
            m_timestamp = osmium::timestamp::parse_iso(timestamp);
            return *this;
        }

        const char* user() const {
            return self() + sizeof_object() + sizeof(size_t);
        }

        TagList& tags() {
            return subitem_of_type<TagList>();
        }

        const TagList& tags() const {
            return subitem_of_type<const TagList>();
        }

        /**
         * Set named attribute.
         * @param attr Name of the attribute (must be one of "id", "version", "changeset", "timestamp", "uid", "visible")
         * @param value Value of the attribute
         */
        void set_attribute(const char* attr, const char* value) {
            if (!strcmp(attr, "id")) {
                id(value);
            } else if (!strcmp(attr, "version")) {
                version(value);
            } else if (!strcmp(attr, "changeset")) {
                changeset(value);
            } else if (!strcmp(attr, "timestamp")) {
                timestamp(value);
            } else if (!strcmp(attr, "uid")) {
                uid(value);
            } else if (!strcmp(attr, "visible")) {
                visible(value);
            }
        }

        typedef osmium::memory::CollectionIterator<Item> iterator;
        typedef osmium::memory::CollectionIterator<const Item> const_iterator;

        iterator begin() {
            return iterator(subitems_position());
        }

        iterator end() {
            return iterator(self() + padded_size());
        }

        const_iterator cbegin() const {
            return const_iterator(subitems_position());
        }

        const_iterator cend() const {
            return const_iterator(self() + padded_size());
        }

        const_iterator begin() const {
            return cbegin();
        }

        const_iterator end() const {
            return cend();
        }

    }; // class Object

    static_assert(sizeof(Object) % osmium::memory::align_bytes == 0, "Class osmium::Object has wrong size to be aligned properly!");

    /**
     * Objects can be ordered by id and version.
     * Note that we use the absolute value of the id for a
     * better ordering of objects with negative id.
     */
    inline bool operator<(const Object& lhs, const Object& rhs) {
        return (lhs.id() == rhs.id() && lhs.version() < rhs.version()) || abs(lhs.id()) < abs(rhs.id());
    }

} // namespace osmium

#endif // OSMIUM_OSM_OBJECT_HPP