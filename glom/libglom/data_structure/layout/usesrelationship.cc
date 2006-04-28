/* Glom
 *
 * Copyright (C) 2001-2004 Murray Cumming
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "fieldformatting.h"
#include <glibmm/i18n.h>

UsesRelationship::UsesRelationship()
{
}

UsesRelationship::UsesRelationship(const UsesRelationship& src)
: m_relationship(src.m_relationship),
  m_related_relationship(src.m_related_relationship)
{
}

UsesRelationship::~UsesRelationship()
{
}

bool UsesRelationship::operator==(const UsesRelationship& src) const
{
  return (m_relationship == src.m_relationship) 
         && (m_related_relationship == src.m_related_relationship);
}


UsesRelationship& UsesRelationship::operator=(const UsesRelationship& src)
{
  m_relationship = src.m_relationship;
  m_related_relationship = src.m_related_relationship;

  return *this;
}

bool UsesRelationship::get_has_relationship_name() const
{
  if(!m_relationship)
    return false;
  else
    return !(m_relationship->get_name().empty());
}

bool UsesRelationship::get_has_related_relationship_name() const
{
  if(!m_related_relationship)
    return false;
  else
    return !(m_related_relationship->get_name().empty());
}


Glib::ustring UsesRelationship::get_relationship_name() const
{
  if(m_relationship)
    return m_relationship->get_name();
  else
    return Glib::ustring();
}

Glib::ustring UsesRelationship::get_related_relationship_name() const
{
  if(m_related_relationship)
    return m_related_relationship->get_name();
  else
    return Glib::ustring();
}

sharedptr<Relationship> UsesRelationship::get_relationship() const
{
  return m_relationship;
}

void UsesRelationship::set_relationship(const sharedptr<Relationship>& relationship)
{
  m_relationship = relationship;
}

sharedptr<Relationship> UsesRelationship::get_related_relationship() const
{
  return m_related_relationship;
}

void UsesRelationship::set_related_relationship(const sharedptr<Relationship>& relationship)
{
  m_related_relationship = relationship;
}

Glib::ustring UsesRelationship::get_sql_table_or_join_alias_name(const Glib::ustring& parent_table) const
{
  if(get_has_relationship_name() || get_has_related_relationship_name())
  {
    const Glib::ustring result = get_sql_join_alias_name();
    if(result.empty())
    {
      //Non-linked-fields relationship:
      return get_table_used(parent_table);
    }
    else
      return result;
  }
  else
    return parent_table;
}

Glib::ustring UsesRelationship::get_table_used(const Glib::ustring& parent_table) const
{
  if(m_related_relationship)
    return m_related_relationship->get_to_table();
  else if(m_relationship)
    return m_relationship->get_to_table();
  else
    return parent_table;
}

Glib::ustring UsesRelationship::get_sql_join_alias_name() const
{
  Glib::ustring result;

  if(get_has_relationship_name() && m_relationship->get_has_fields()) //relationships that link to tables together via a field
  {
    //We use relationship_name.field_name instead of related_table_name.field_name,
    //because, in the JOIN below, will specify the relationship_name as an alias for the related table name
    result += ("relationship_" + m_relationship->get_name());

    /*
    const Glib::ustring field_table_name = relationship->get_to_table();
    if(field_table_name.empty())
    {
      g_warning("build_sql_select_with_where_clause(): field_table_name is null. relationship name = %s", relationship->get_name().c_str());
    }
    */

    if(get_has_related_relationship_name() && m_related_relationship->get_has_fields())
    {
      result += ("_" + m_related_relationship->get_name());
    }
  }

  return result;
}

Glib::ustring UsesRelationship::get_sql_join_alias_definition() const
{
  Glib::ustring result;

  if(!get_has_related_relationship_name())
  {
    result = " LEFT OUTER JOIN \"" + m_relationship->get_to_table() + "\""
             + " AS \"" + get_sql_join_alias_name() + "\"" //Specify an alias, to avoid ambiguity when using 2 relationships to the same table.
             + " ON (\"" + m_relationship->get_from_table() + "\".\"" + m_relationship->get_from_field() + "\" = \""
             + get_sql_join_alias_name() + "\".\"" + m_relationship->get_to_field() + "\")";
  }
  else
  {
     UsesRelationship parent_relationship;
     parent_relationship.set_relationship(m_relationship);
     result = " LEFT OUTER JOIN \"" + m_related_relationship->get_to_table() + "\""
             + " AS \"" + get_sql_join_alias_name() + "\"" //Specify an alias, to avoid ambiguity when using 2 relationships to the same table.
             + " ON (\"" + parent_relationship.get_sql_join_alias_name() + "\".\"" + m_related_relationship->get_from_field() + "\" = \""
             + get_sql_join_alias_name() + "\".\"" + m_related_relationship->get_to_field() + "\")";
  }
  return result;
}



