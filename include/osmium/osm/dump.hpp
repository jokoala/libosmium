#ifndef OSMIUM_OSM_DUMP_HPP
#define OSMIUM_OSM_DUMP_HPP

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

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>

#include <osmium/osm/types.hpp>
#include <osmium/osm/ostream.hpp>
#include <osmium/osm/visitor.hpp>
#include <osmium/utils/timestamp.hpp>

namespace osmium {

    namespace osm {

        class Dump {

            std::ostream& m_out;
            bool m_with_size;
            std::string m_prefix;

            void print_title(const char* title, const osmium::memory::Item& item) {
                m_out << m_prefix
                      << title
                      << ":";

                if (m_with_size) {
                    m_out << " ["
                          << item.size()
                          << "]";
                }

                m_out << "\n";
            }

            void print_meta(const osmium::Object& object) {
                m_out << m_prefix
                      << "  id="
                      << object.id()
                      << "\n";
                m_out << m_prefix
                      << "  version="
                      << object.version()
                      << "\n";
                m_out << m_prefix
                      << "  uid="
                      << object.uid()
                      << "\n";
                m_out << m_prefix
                      << "  user=|"
                      << object.user()
                      << "|\n";
                m_out << m_prefix
                      << "  changeset="
                      << object.changeset()
                      << "\n";
                m_out << m_prefix
                      << "  timestamp="
                      << osmium::timestamp::to_iso(object.timestamp())
                      << "\n";
                m_out << m_prefix
                      << "  visible="
                      << (object.visible() ? "yes" : "no")
                      << "\n";

                Dump dump(m_out, m_with_size, m_prefix + "  ");
                osmium::osm::apply_visitor(dump, object.cbegin(), object.cend());
            }

            void print_location(const osmium::Node& node) {
                const osmium::Location& location = node.location();

                m_out << m_prefix
                      << "  lon="
                      << std::fixed
                      << std::setprecision(7)
                      << location.lon()
                      << "\n";
                m_out << m_prefix
                      << "  lat="
                      << std::fixed
                      << std::setprecision(7)
                      << location.lat()
                      << "\n";
            }

        public:

            Dump(std::ostream& out, bool with_size=true, const std::string& prefix="") :
                m_out(out),
                m_with_size(with_size),
                m_prefix(prefix) {
            }

            void operator()(const osmium::TagList& tags) {
                print_title("TAGS", tags);
                for (auto& tag : tags) {
                    m_out << m_prefix
                          << "  k=|"
                          << tag.key()
                          << "| v=|"
                          << tag.value()
                          << "|"
                          << "\n";
                }
            }

            void operator()(const osmium::WayNodeList& wnl) {
                print_title("NODES", wnl);
                for (auto& wn : wnl) {
                    m_out << m_prefix
                          << "  ref="
                          << wn.ref();
                    if (wn.location().defined()) {
                        m_out << " pos="
                              << wn.location();
                    }
                    m_out << "\n";
                }
            }

            void operator()(const osmium::RelationMemberList& rml) {
                print_title("MEMBERS", rml);
                for (auto& member : rml) {
                    m_out << m_prefix
                          << "  type="
                          << member.type()
                          << " ref="
                          << member.ref()
                          << " role=|"
                          << member.role()
                          << "|\n";
                    if (member.full_member()) {
                        Dump dump(m_out, m_with_size, m_prefix + "  | ");
                        osmium::osm::apply_visitor(dump, member.get_object());
                    }
                }
            }

            void operator()(const osmium::Node& node) {
                print_title("NODE", node);
                print_meta(node);
                print_location(node);
            }

            void operator()(const osmium::Way& way) {
                print_title("WAY", way);
                print_meta(way);
            }

            void operator()(const osmium::Relation& relation) {
                print_title("RELATION", relation);
                print_meta(relation);
            }

        }; // class Dump

    } // namespace osm

} // namespace osmium

#endif // OSMIUM_OSM_DUMP_HPP
