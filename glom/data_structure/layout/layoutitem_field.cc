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
 
#include "layoutitem_field.h"
#include <glibmm/i18n.h>

LayoutItem_Field::LayoutItem_Field()
: m_priv_view(false),
  m_priv_edit(false),
  m_field_cache_valid(false),
  m_hidden(false),
  m_formatting_use_default(true)
{
}

LayoutItem_Field::LayoutItem_Field(const LayoutItem_Field& src)
: LayoutItem(src),
  m_priv_view(src.m_priv_view),
  m_priv_edit(src.m_priv_edit),
  //m_table_name(src.m_table_name),
  m_relationship(src.m_relationship),
  m_formatting(src.m_formatting),
  m_field_cache_valid(src.m_field_cache_valid),
  m_hidden(src.m_hidden),
  m_formatting_use_default(src.m_formatting_use_default)
{
//g_warning("LayoutItem_Field::LayoutItem_Field: m_choices_related_relationship=%s, src.m_choices_related_relationship=%s", m_choices_related_relationship.c_str(), src.m_choices_related_relationship.c_str());

  m_field = src.m_field;
}

LayoutItem_Field::~LayoutItem_Field()
{
}

LayoutItem* LayoutItem_Field::clone() const
{
  return new LayoutItem_Field(*this);
}

bool LayoutItem_Field::operator==(const LayoutItem_Field& src) const
{
  bool result = LayoutItem::operator==(src) &&
    (m_priv_view == src.m_priv_view) &&
    (m_priv_edit == src.m_priv_edit) &&
    (m_relationship == src.m_relationship) &&
    (m_hidden == src.m_hidden) &&
    (m_formatting_use_default == src.m_formatting_use_default) &&
    (m_formatting == src.m_formatting) &&
    (m_field_cache_valid == src.m_field_cache_valid);

  if(m_field && src.m_field)
    result == result && (*m_field == *(src.m_field));
  else
    result = result && (m_field == src.m_field);

  return result;
}

//Avoid using this, for performance:
LayoutItem_Field& LayoutItem_Field::operator=(const LayoutItem_Field& src)
{
  LayoutItem::operator=(src);

  m_field = src.m_field;
  m_field_cache_valid = src.m_field_cache_valid;

  m_priv_view = src.m_priv_view;
  m_priv_edit = src.m_priv_edit;

  m_relationship = src.m_relationship;

  m_hidden = src.m_hidden;

  m_formatting_use_default = src.m_formatting_use_default;
  m_formatting = src.m_formatting;

  return *this;
}

void LayoutItem_Field::set_name(const Glib::ustring& name)
{
  if(get_name() != name)
    m_field_cache_valid = false;

  LayoutItem::set_name(name);
}

Glib::ustring LayoutItem_Field::get_name() const
{
  return LayoutItem::get_name();
}

Glib::ustring LayoutItem_Field::get_title_or_name() const
{
  if(m_field_cache_valid && m_field)
    return m_field->get_title_or_name();
  else
    return get_name();
}

bool LayoutItem_Field::get_has_relationship_name() const
{
  return m_relationship.get_name_not_empty();
}

Glib::ustring LayoutItem_Field::get_relationship_name() const
{
  return m_relationship.get_name();
}

bool LayoutItem_Field::get_editable_and_allowed() const
{
  //Thre relationship might forbid editing of any fields through itself:
  if(get_has_relationship_name())
    if(!(m_relationship.get_allow_edit()))
      return false; 

  return m_editable && m_priv_edit;
}

Glib::ustring LayoutItem_Field::get_layout_display_name() const
{
  Glib::ustring result;

  if(m_field_cache_valid && m_field)
    result= m_field->get_name();

  if(get_has_relationship_name())
    result == get_relationship_name() + "::" + result;

  return result;
}

bool LayoutItem_Field::get_hidden() const
{
  return m_hidden;
}

void LayoutItem_Field::set_hidden(bool val)
{
  m_hidden = val;
}

Glib::ustring LayoutItem_Field::get_part_type_name() const
{
  return _("Field");
}

bool LayoutItem_Field::get_formatting_use_default() const
{
  return m_formatting_use_default;
}

void LayoutItem_Field::set_formatting_use_default(bool use_default)
{
  m_formatting_use_default = use_default;
}

const FieldFormatting& LayoutItem_Field::get_formatting_used() const
{
  if(m_formatting_use_default && m_field_cache_valid && m_field)
    return m_field->m_default_formatting;
  else
    return m_formatting;
}

void LayoutItem_Field::set_full_field_details(const sharedptr<const Field>& field)
{

  if(field)
  {
    //std::cout << "LayoutItem_Field::set_full_field_details(): name=" << field->get_name() << std::endl;
    //std::cout << "LayoutItem_Field::set_full_field_details(): field->get_title_or_name()=" << field->get_title_or_name() << std::endl;
    m_field = field;
    m_field_cache_valid = true;

    LayoutItem::set_name(field->get_name()); //It seems to be OK to expect get_name() to work after setting _full_ details.
  }
  else
  {
    //std::cout << "LayoutItem_Field::set_full_field_details(null): previous name=" << m_name << std::endl;
    m_field = sharedptr<const Field>();
    m_field_cache_valid = false;
  }
}

sharedptr<const Field> LayoutItem_Field::get_full_field_details() const
{
  return m_field;
}

Field::glom_field_type LayoutItem_Field::get_glom_type() const
{
  if(m_field && m_field_cache_valid)
    return m_field->get_glom_type();
  else
    return Field::TYPE_INVALID;
}





