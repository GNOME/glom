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

#ifndef GLOM_DATASTRUCTURE_LAYOUTITEM_FIELD_H
#define GLOM_DATASTRUCTURE_LAYOUTITEM_FIELD_H

#include "layoutitem.h"
#include "usesrelationship.h"
#include "../field.h"
#include "../numeric_format.h"
#include "../relationship.h"
#include "custom_title.h"

namespace Glom
{
  
/** A predicate for use with std::find_if() to find a LayoutItem_Field which refers 
 * to the same field, without comparing irrelevant stuff such as formatting.
 */
template<class T_Element>
class predicate_LayoutItem_Field_IsSameField
{
public:
  predicate_LayoutItem_Field_IsSameField(const sharedptr<const T_Element>& layout_item)
  {
    m_layout_item = layout_item;
  }

  virtual ~predicate_LayoutItem_Field_IsSameField()
  {
  }

  bool operator() (const sharedptr<const T_Element>& element)
  {
    const bool result = (m_layout_item->get_name() == element->get_name());
    
    if(!result)
      return false;
    
    //Compare the relationship and related relationshp:
    sharedptr<const UsesRelationship> uses_a = m_layout_item;
    sharedptr<const UsesRelationship> uses_b = element;
    return (*uses_a == *uses_b);
  }
    
protected:
  sharedptr<const T_Element> m_layout_item;
};

/** A LayoutItem that shows the data from a table field.
 */
class LayoutItem_Field 
 : public LayoutItem,
   public UsesRelationship
{
public:

  LayoutItem_Field();
  LayoutItem_Field(const LayoutItem_Field& src);
  LayoutItem_Field& operator=(const LayoutItem_Field& src);
  virtual ~LayoutItem_Field();

  virtual LayoutItem* clone() const;

  bool operator==(const LayoutItem_Field& src) const;

  virtual void set_name(const Glib::ustring& name);
  virtual Glib::ustring get_name() const; //For use with our std::find_if() predicate.

  virtual Glib::ustring get_title_or_name() const;

  Glib::ustring get_title_or_name_no_custom() const;

  sharedptr<const CustomTitle> get_title_custom() const;
  sharedptr<CustomTitle> get_title_custom();
  void set_title_custom(const sharedptr<CustomTitle>& title);

  //virtual Glib::ustring get_table_name() const;
  //virtual void set_table_name(const Glib::ustring& table_name);

  /** Get a text representation for the field, such as Relationship::FieldName.
   */
  virtual Glib::ustring get_layout_display_name() const;

  virtual Glib::ustring get_part_type_name() const;

  virtual Glib::ustring get_report_part_id() const;

  void set_full_field_details(const sharedptr<const Field>& field);
  sharedptr<const Field> get_full_field_details() const;

  ///Convenience function, to avoid use of get_full_field_details().
  Field::glom_field_type get_glom_type() const;

  //TODO: This might occasionally be different on different layouts: Glib::ustring m_title;


  bool get_editable_and_allowed() const;

  /// For extra fields, needed for SQL queries. The user should never be able to make an item hidden - he can just remove it.
  bool get_hidden() const;
  void set_hidden(bool val = true);

  //Not saved to the document:
  bool m_priv_view;
  bool m_priv_edit;

  FieldFormatting m_formatting;

  bool get_formatting_use_default() const;
  void set_formatting_use_default(bool use_default = true);

  const FieldFormatting& get_formatting_used() const;

  /** Compare the name, relationship, and related_relationship.
   */
  bool is_same_field(const sharedptr<const LayoutItem_Field>& field) const;

  /** Returns, for instance, "mytable"."myfield". 
   */
  Glib::ustring get_sql_name(const Glib::ustring& parent_table) const;

protected:

  //This is just a cache, filled in by looking at the database structure:
  sharedptr<const Field> m_field;
  bool m_field_cache_valid; //Whetehr m_field is up-to-date.

  bool m_hidden;
  bool m_formatting_use_default;
  sharedptr<CustomTitle> m_title_custom; //translatable.
};

} //namespace Glom

#endif //GLOM_DATASTRUCTURE_LAYOUTITEM_FIELD_H



