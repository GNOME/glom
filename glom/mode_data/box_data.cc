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

#include "box_data.h"
#include "../data_structure/glomconversions.h"
#include "../utils.h"
#include "../data_structure/layout/layoutitem_field.h"
#include "../python_embed/glom_python.h"
#include <bakery/App/App_Gtk.h> //For util_bold_message().
#include <algorithm> //For std::find()
#include "config.h"
#include <glibmm/i18n.h>

Box_Data::Box_Data()
: m_Button_Find(Gtk::Stock::FIND),
  m_pDialogLayout(0)
{
  m_bUnstoredData = false;

  //Connect signals:
  m_Button_Find.signal_clicked().connect(sigc::mem_fun(*this, &Box_Data::on_Button_Find));
}

Box_Data::~Box_Data()
{
  if(m_pDialogLayout)
  {
    remove_view(m_pDialogLayout);
    delete m_pDialogLayout;
  }
}

bool Box_Data::init_db_details(const Glib::ustring& strTableName, const Glib::ustring& strWhereClause)
{
  m_strTableName = strTableName;
  m_strWhereClause = strWhereClause;

  create_layout(); //So that fill_from_database() can succeed.

  return Box_DB_Table::init_db_details(strTableName); //Calls fill_from_database().
}

bool Box_Data::refresh_data_from_database_with_where_clause(const Glib::ustring& strWhereClause)
{
  m_strWhereClause = strWhereClause;

  return Box_DB_Table::refresh_data_from_database(); //Calls fill_from_database().
}

Glib::ustring Box_Data::get_where_clause() const
{
  return m_strWhereClause;
}

Glib::ustring Box_Data::get_find_where_clause() const
{
  Glib::ustring strClause;

  //Look at each field entry and build e.g. 'Name = "Bob"'
  for(type_vecLayoutFields::const_iterator iter = m_FieldsShown.begin(); iter != m_FieldsShown.end(); ++iter)
  {
    Glib::ustring strClausePart;

    const Gnome::Gda::Value data = get_entered_field_data(*iter);

    if(!GlomConversions::value_is_empty(data))
    {
      const sharedptr<const Field> field = (*iter)->get_full_field_details();
      if(field)
      {
        bool use_this_field = true;
        if(field->get_glom_type() == Field::TYPE_BOOLEAN) //TODO: We need an intermediate state for boolean fields, so that they can be ignored in searches.
        {
          if(!data.get_bool())
            use_this_field = false;
        }
  
        if(use_this_field)
        {
          strClausePart = m_strTableName + "." + field->get_name() + " " + field->sql_find_operator() + " " +  field->sql_find(data); //% is mysql wildcard for 0 or more characters.
        }
      }
    }

    if(!strClausePart.empty())
      strClause += strClausePart + " ";
  }

  return strClause;
}

void Box_Data::on_Button_Find()
{
  //Make sure that the cell is updated:
  //m_AddDel.finish_editing();

  const Glib::ustring where_clause = get_find_where_clause();
  if(where_clause.empty())
  {
    Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("No Find Criteria")), true, Gtk::MESSAGE_WARNING );
    dialog.set_secondary_text(_("You have not entered any find criteria. Try entering information in the fields."));
    dialog.set_transient_for(*get_app_window());
    dialog.run();
  }
  else
    signal_find_criteria.emit(where_clause);
}

Box_Data::type_map_fields Box_Data::get_record_field_values(const Gnome::Gda::Value& primary_key_value)
{
  type_map_fields field_values;

  Document_Glom* document = get_document();
  if(document)
  {
    //TODO: Cache the list of all fields, as well as caching (m_Fields) the list of all visible fields:
    const Document_Glom::type_vecFields fields = document->get_table_fields(m_strTableName);

    //TODO: This seems silly. We should just have a build_sql_select() that can take this container:
    type_vecLayoutFields fieldsToGet;
    for(Document_Glom::type_vecFields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
    {
      sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
      layout_item->set_full_field_details(*iter);

      fieldsToGet.push_back(layout_item);
    }

    if(!GlomConversions::value_is_empty(primary_key_value))
    {
      sharedptr<const Field> fieldPrimaryKey = get_field_primary_key();

      const Glib::ustring query = build_sql_select(m_strTableName, fieldsToGet, fieldPrimaryKey, primary_key_value);
      Glib::RefPtr<Gnome::Gda::DataModel> data_model = Query_execute(query);

      if(data_model && data_model->get_n_rows())
      {
        int col_index = 0;
        for(Document_Glom::type_vecFields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
        {
          //There should be only 1 row. Well, there could be more but we will ignore them.
          sharedptr<const Field> field = *iter;

          Gnome::Gda::Value value = data_model->get_value_at(col_index, 0);

          //Never give a NULL-type value to the python calculation,
          //to prevent errors:
          if(value.get_value_type() == Gnome::Gda::VALUE_TYPE_NULL)
            value = GlomConversions::get_empty_value_suitable_for_python(field->get_glom_type());

          field_values[field->get_name()] = value;
          ++col_index;
        }
      }
      else
      {
        handle_error();
      }
    }

    if(field_values.empty()) //Maybe there was no primary key, or maybe the record is not yet in the database.
    {
      //Create appropriate empty values:
      for(Document_Glom::type_vecFields::const_iterator iter = fields.begin(); iter != fields.end(); ++iter)
      {
        sharedptr<const Field> field = *iter;
        field_values[field->get_name()] = GlomConversions::get_empty_value_suitable_for_python(field->get_glom_type());
      }
    }
  }

  return field_values;
}

Glib::RefPtr<Gnome::Gda::DataModel> Box_Data::record_new(bool use_entered_data, const Gnome::Gda::Value& primary_key_value)
{
  sharedptr<const Field> fieldPrimaryKey = get_field_primary_key();

  const Glib::ustring primary_key_name = fieldPrimaryKey->get_name();

  type_vecLayoutFields fieldsToAdd = m_FieldsShown;

  //Add values for all fields, not just the shown ones:
  //For instance, we must always add the primary key, and fields with default/calculated/lookup values:
  for(type_vecFields::const_iterator iter = m_TableFields.begin(); iter != m_TableFields.end(); ++iter)
  {
    //TODO: Search for the non-related field with the name, not just the field with the name:
    type_vecLayoutFields::const_iterator iterFind = std::find_if(fieldsToAdd.begin(), fieldsToAdd.end(), predicate_FieldHasName<LayoutItem_Field>((*iter)->get_name()));
    if(iterFind == fieldsToAdd.end())
    {
      sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
      layout_item->set_full_field_details(*iter);

      fieldsToAdd.push_back(layout_item);
    }
  }

  //Calculate any necessary field values and enter them:
  for(type_vecLayoutFields::const_iterator iter = fieldsToAdd.begin(); iter != fieldsToAdd.end(); ++iter)
  {
    sharedptr<LayoutItem_Field> layout_item = *iter;

    //If the user did not enter something in this field:
    Gnome::Gda::Value value = get_entered_field_data(layout_item);

    if(GlomConversions::value_is_empty(value)) //This deals with empty strings too.
    {
      const sharedptr<const Field>& field = layout_item->get_full_field_details();
      if(field)
      {
        //If the default value should be calculated, then calculate it:
        if(field->get_has_calculation())
        {
          const Glib::ustring calculation = field->get_calculation();
          const type_map_fields field_values = get_record_field_values(primary_key_value);
  
          const Gnome::Gda::Value value = glom_evaluate_python_function_implementation(field->get_glom_type(), calculation, field_values,
            get_document(), m_strTableName);
          set_entered_field_data(layout_item, value);
        }
  
        //Use default values (These are also specified in postgres as part of the field definition,
        //so we could theoretically just not specify it here.
        //TODO_Performance: Add has_default_value()?
        if(GlomConversions::value_is_empty(value))
        {
          const Gnome::Gda::Value default_value = field->get_default_value();
          if(!GlomConversions::value_is_empty(default_value))
          {
            set_entered_field_data(layout_item, default_value);
          }
        }
      }
    }
  }

  //Get all entered field name/value pairs:
  Glib::ustring strNames;
  Glib::ustring strValues;

  //Avoid specifying the same field twice:
  typedef std::map<Glib::ustring, bool> type_map_added;
  type_map_added map_added;

  for(type_vecLayoutFields::const_iterator iter = fieldsToAdd.begin(); iter != fieldsToAdd.end(); ++iter)
  {
    sharedptr<LayoutItem_Field> layout_item = *iter;
    const Glib::ustring field_name = layout_item->get_name();
    if(!layout_item->get_has_relationship_name()) //TODO: Allow people to add a related record also by entering new data in a related field of the related record.
    {
      type_map_added::const_iterator iterFind = map_added.find(field_name);
      if(iterFind == map_added.end()) //If it was not added already
      {
        Gnome::Gda::Value value;

        const sharedptr<const Field>& field = layout_item->get_full_field_details();
        if(field)
        {
          //Use the specified (generated) primary key value, if there is one:
          if(primary_key_name == field_name && !GlomConversions::value_is_empty(primary_key_value))
          {
            value = primary_key_value;
          }
          else
          {
            if(use_entered_data || !field->get_calculation().empty()) //TODO_Performance: Use a get_has_calculation() method.
              value = get_entered_field_data(layout_item);
          }
  
          Glib::ustring strFieldValue = field->sql(value);
  
          if(!strFieldValue.empty())
          {
            if(!strNames.empty())
            {
              strNames += ", ";
              strValues += ", ";
            }
  
            strNames += "\"" + field_name + "\"";
            strValues += strFieldValue;
  
            map_added[field_name] = true;
          }
        }
      }
    }
  }

  //Put it all together to create the record with these field values:
  if(!strNames.empty() && !strValues.empty())
  {
    Glib::ustring strQuery = "INSERT INTO \"" + m_strTableName + "\" (" + strNames + ") VALUES (" + strValues + ")";
    return Query_execute(strQuery);
  }
  else
    return Glib::RefPtr<Gnome::Gda::DataModel>();
}

void Box_Data::set_unstored_data(bool bVal)
{
  m_bUnstoredData = bVal;
}

bool Box_Data::get_unstored_data() const
{
  return m_bUnstoredData;
}

void Box_Data::create_layout()
{
  set_unstored_data(false);

  //Cache the table information, for performance:
  m_TableFields = get_fields_for_table(m_strTableName);
}

bool Box_Data::fill_from_database()
{
  set_unstored_data(false);

  return Box_DB_Table::fill_from_database();
}

bool Box_Data::confirm_discard_unstored_data() const
{
  if(get_unstored_data())
  {
    //Ask user to confirm loss of data:
    Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("No primary key value")), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_OK_CANCEL );
    dialog.set_secondary_text(_("This data cannot be stored in the database because you have not provided a primary key.\nDo you really want to discard this data?"));
    //TODO: It needs a const. I wonder if it should. murrayc. dialog.set_transient_for(*get_app_window());
    int iButton = dialog.run();

    return (iButton == Gtk::RESPONSE_OK);
  }
  else
  {
    return true; //no data to lose.
  }
}

void Box_Data::show_layout_dialog()
{
  if(m_pDialogLayout)
  {
    m_pDialogLayout->set_document(m_layout_name, get_document(), m_strTableName, m_FieldsShown); //TODO: Use m_TableFields?
    m_pDialogLayout->show();
  }
}

void Box_Data::on_dialog_layout_hide()
{
  //Re-fill view, in case the layout has changed:
  create_layout();

  if(ConnectionPool::get_instance()->get_ready_to_connect())
    fill_from_database();
}

Box_Data::type_vecLayoutFields Box_Data::get_fields_to_show() const
{
  if(m_strTableName.empty())
  {
    return type_vecLayoutFields();
  }
  else
    return get_table_fields_to_show(m_strTableName);
}

Box_Data::type_vecLayoutFields Box_Data::get_table_fields_to_show(const Glib::ustring& table_name) const
{
  const Document_Glom* pDoc = dynamic_cast<const Document_Glom*>(get_document());
  if(pDoc)
  {
    Document_Glom::type_mapLayoutGroupSequence mapGroupSequence =  pDoc->get_data_layout_groups_plus_new_fields(m_layout_name, table_name);
    return get_table_fields_to_show_for_sequence(table_name, mapGroupSequence);
  }
  else
    return type_vecLayoutFields();
}

Gnome::Gda::Value Box_Data::generate_next_auto_increment(const Glib::ustring& table_name, const Glib::ustring field_name)
{
  //Get it from the database system table.
  //The developer can change the next value in the Database Preferences
  return get_next_auto_increment_value(table_name, field_name);

  /*
  //This is a workaround for postgres problems. Ideally, we need to use the postgres serial type and find out how to get the generated value after we add a row.

  double result = 0;
  Glib::RefPtr<Gnome::Gda::DataModel> data_model = Query_execute("SELECT max(" + field_name + ") FROM \"" + table_name + "\"");
  if(!data_model && data_model->get_n_rows())
    handle_error();
  else
  {
    //The result should be 1 row with 1 column
    Gnome::Gda::Value value = data_model->get_value_at(0, 0);

    //It's a Gnome::Gda::Value::TYPE_NUMERIC, but the GdaNumeric struct is not easy to handle, so let's hack around it:
    //if(value.is_number())
    //  result = value.get_integer();
    //else
    result = util_decimal_from_string(value.to_string());

    ++result; 
  }

  //Get a string representation of the number, so we can put it back in a NUMERIC Gda::Value:

  return GlomConversions::parse_value(result);
  */
}

/** Get the shown fields that are in related tables, via a relationship using @a field_name changes.
 */
Box_Data::type_vecLayoutFields Box_Data::get_related_fields(const Glib::ustring& field_name) const
{
  type_vecLayoutFields result;

  const Document_Glom* document = dynamic_cast<const Document_Glom*>(get_document());
  if(document)
  {
    for(type_vecLayoutFields::const_iterator iter = m_FieldsShown.begin(); iter != m_FieldsShown.end();  ++iter)
    {
      sharedptr<LayoutItem_Field> layout_field = *iter;
      //Examine each field that looks up its data from a relationship:
      if(layout_field->get_has_relationship_name())
      {
        //Get the relationship information:
        sharedptr<const Relationship> relationship = document->get_relationship(m_strTableName, layout_field->get_relationship_name());
        if(relationship)
        {
          //If the relationship uses the specified field:
          if(relationship->get_from_field() == field_name)
          {
            //Add it:
            result.push_back(layout_field);
          }
        }
      }
    }
  }

  return result;
}

Box_Data::type_field_calcs Box_Data::get_calculated_fields(const Glib::ustring& field_name)
{
  type_field_calcs result;

  const Document_Glom* document = dynamic_cast<const Document_Glom*>(get_document());
  if(document)
  {
    //Examine all fields, not just the the shown fields.
    for(type_vecFields::const_iterator iter = m_TableFields.begin(); iter != m_TableFields.end();  ++iter)
    {
      sharedptr<const Field> field = *iter;

      //Does this field's calculation use the field?
      const Field::type_list_strings fields_triggered = field->get_calculation_fields();
      Field::type_list_strings::const_iterator iterFind = std::find(fields_triggered.begin(), fields_triggered.end(), field_name);
      if(iterFind != fields_triggered.end())
      {
        CalcInProgress item;
        item.m_field = field;

        result[field->get_name()] = item;
      }
    }
  }

  return result;
}

/** Get the fields whose values should be looked up when @a field_name changes, with
 * the relationship used to lookup the value.
 */
Box_Data::type_list_lookups Box_Data::get_lookup_fields(const Glib::ustring& field_name) const
{
  type_list_lookups result;

  const Document_Glom* document = dynamic_cast<const Document_Glom*>(get_document());
  if(document)
  {
    //Examine all fields, not just the the shown fields (m_Fields):
    for(type_vecFields::const_iterator iter = m_TableFields.begin(); iter != m_TableFields.end();  ++iter)
    {
      sharedptr<Field> field = *iter;

      //Examine each field that looks up its data from a relationship:
      if(field->get_is_lookup())
      {
        //Get the relationship information:
        sharedptr<Relationship> relationship = field->get_lookup_relationship();
        if(relationship)
        {
          //If the relationship is triggererd by the specified field:
          if(relationship->get_from_field() == field_name)
          {
            //Add it:
            sharedptr<LayoutItem_Field> item = sharedptr<LayoutItem_Field>::create();
            item->set_full_field_details(field);
            result.push_back( type_pairFieldTrigger(item, relationship) );
          }
        }
      }
    }
  }

  return result;
}

Gnome::Gda::Value Box_Data::get_lookup_value(const sharedptr<const Relationship>& relationship, const sharedptr<const Field>& source_field, const Gnome::Gda::Value& key_value)
{
  Gnome::Gda::Value result;

  sharedptr<Field> to_key_field = get_fields_for_table_one_field(relationship->get_to_table(), relationship->get_to_field());
  if(to_key_field)
  {
    //Convert the value, in case the from and to fields have different types:
    const Gnome::Gda::Value value_to_key_field = GlomConversions::convert_value(key_value, to_key_field->get_glom_type());

    Glib::ustring strQuery = "SELECT \"" + relationship->get_to_table() + "\".\"" + source_field->get_name() + "\" FROM \"" +  relationship->get_to_table() + "\"";
    strQuery += " WHERE \"" + to_key_field->get_name() + "\" = " + to_key_field->sql(value_to_key_field);

    Glib::RefPtr<Gnome::Gda::DataModel> data_model = Query_execute(strQuery);
    if(data_model && data_model->get_n_rows())
    {
      //There should be only 1 row. Well, there could be more but we will ignore them.
      result = data_model->get_value_at(0, 0);
    }
    else
    {
      handle_error();
    }
  }

  return result;
}

void Box_Data::calculate_field(const sharedptr<const Field>& field, const sharedptr<const Field>& primary_key, const Gnome::Gda::Value& primary_key_value)
{
  const Glib::ustring field_name = field->get_name();
  //g_warning("Box_Data::calculate_field(): field_name=%s", field_name.c_str());

  //Do we already have this in our list?
  type_field_calcs::iterator iterFind = m_FieldsCalculationInProgress.find(field_name);
  if(iterFind == m_FieldsCalculationInProgress.end()) //If it was not found.
  {
    //Add it:
    CalcInProgress item;
    item.m_field = field;
    m_FieldsCalculationInProgress[field_name] = item;

    iterFind = m_FieldsCalculationInProgress.find(field_name); //Always succeeds.
  }

  CalcInProgress& refCalcProgress = iterFind->second;

  //Use the previously-calculated value if possible:
  if(refCalcProgress.m_calc_in_progress)
  {
    //g_warning("  Box_Data::calculate_field(): Circular calculation detected. field_name=%s", field_name.c_str());
    //refCalcProgress.m_value = GlomConversions::get_empty_value(field->get_glom_type()); //Give up.
  }
  else if(refCalcProgress.m_calc_finished)
  {
    //g_warning("  Box_Data::calculate_field(): Already calculated.");

    //Don't bother calculating it again. The correct value is already in the database and layout.
  }
  else
  {
    //g_warning("  Box_Data::calculate_field(): setting calc_in_progress: field_name=%s", field_name.c_str());

    refCalcProgress.m_calc_in_progress = true; //Let the recursive calls to calculate_field() check this.

    sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
    layout_item->set_full_field_details(refCalcProgress.m_field);

    //Calculate dependencies first:
    const Field::type_list_strings fields_needed = field->get_calculation_fields();
    for(Field::type_list_strings::const_iterator iterNeeded = fields_needed.begin(); iterNeeded != fields_needed.end(); ++iterNeeded)
    {
      sharedptr<const Field> field_needed = get_document()->get_field(m_strTableName, *iterNeeded);
      if(field_needed)
      {
        if(field_needed->get_has_calculation())
        {
          //g_warning("  calling calculate_field() for %s", iterNeeded->c_str());
          calculate_field(field_needed, primary_key, primary_key_value);
        }
        else
        {
          //g_warning("  not a calculated field->");
        }
      }
    }


    //m_FieldsCalculationInProgress has changed, probably invalidating our iter, so get it again:
    iterFind = m_FieldsCalculationInProgress.find(field_name); //Always succeeds.
    CalcInProgress& refCalcProgress = iterFind->second;

    //Check again, because the value miight have been calculated during the dependencies.
    if(refCalcProgress.m_calc_finished)
    {
      //We recently calculated this value, and set it in the database and layout, so don't waste time doing it again:
    }
    else
    {
      //recalculate:
      //TODO_Performance: We don't know what fields the python calculation will use, so we give it all of them:
      const type_map_fields field_values = get_record_field_values(primary_key_value);

      sharedptr<const Field> field = refCalcProgress.m_field;
      refCalcProgress.m_value  = glom_evaluate_python_function_implementation(field->get_glom_type(), field->get_calculation(), field_values,
            get_document(), m_strTableName);

      refCalcProgress.m_calc_finished = true;
      refCalcProgress.m_calc_in_progress = false;

      sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
      layout_item->set_full_field_details(field);

      //show it:
      set_entered_field_data(layout_item, refCalcProgress.m_value );

      //Add it to the database (even if it is not shown in the view)
      //Using true for the last parameter means we use existing calculations where possible,
      //instead of recalculating a field that is being calculated already, and for which this dependent field is being calculated anyway.
      set_field_value_in_database(layout_item, refCalcProgress.m_value, primary_key, primary_key_value, true); //This triggers other recalculations/lookups.
    }
  }

}


void Box_Data::do_calculations(const sharedptr<const LayoutItem_Field>& field_changed, const sharedptr<const Field>& primary_key, const Gnome::Gda::Value& primary_key_value, bool first_calc_field)
{
  //g_warning("Box_Data::do_calculations(): triggered by =%s", field_changed->get_name().c_str());

  if(first_calc_field)
  {
    //g_warning("  clearing m_FieldsCalculationInProgress");
    m_FieldsCalculationInProgress.clear();
  }

  //Recalculate fields that are triggered by a change of this field's value, not including calculations that these calculations use.
  type_field_calcs calculated_fields = get_calculated_fields(field_changed->get_name()); //TODO: Just return the Fields, not the CalcInProgress.
  for(type_field_calcs::iterator iter = calculated_fields.begin(); iter != calculated_fields.end(); ++iter)
  {
    sharedptr<const Field> field = iter->second.m_field;

    calculate_field(field, primary_key, primary_key_value); //And any dependencies.

    //Calculate anything that depends on this.
    sharedptr<LayoutItem_Field> layout_item = sharedptr<LayoutItem_Field>::create();
    layout_item->set_full_field_details(field);

    do_calculations(layout_item, primary_key, primary_key_value, false /* recurse, reusing m_FieldsCalculationInProgress */);
  }

  if(first_calc_field)
    m_FieldsCalculationInProgress.clear();
}

bool Box_Data::set_field_value_in_database(const sharedptr<const LayoutItem_Field>& field_layout, const Gnome::Gda::Value& field_value, const sharedptr<const Field>& primary_key_field, const Gnome::Gda::Value& primary_key_value, bool use_current_calculations)
{
  return set_field_value_in_database(Gtk::TreeModel::iterator(), field_layout, field_value, primary_key_field, primary_key_value, use_current_calculations);
}

bool Box_Data::set_field_value_in_database(const Gtk::TreeModel::iterator& row, const sharedptr<const LayoutItem_Field>& field_layout, const Gnome::Gda::Value& field_value, const sharedptr<const Field>& primary_key_field, const Gnome::Gda::Value& primary_key_value, bool use_current_calculations)
{
  //row is invalid, and ignored, for Box_Data_Details.

  const sharedptr<const Field> field = field_layout->get_full_field_details();
  if(!field)
    return false;

  const Glib::ustring field_name = field_layout->get_name();
  if(!field_name.empty()) //This should not happen.
  {
    Glib::ustring table_name;
    if(field_layout->get_has_relationship_name())
    {
      //The field is in a related table.
      sharedptr<Relationship> rel = field_layout->get_relationship();
      if(rel)
        table_name = rel->get_to_table();
    }
    else
      table_name = m_strTableName;

    if(table_name.empty())
    {
      g_warning("Box_Data::set_field_value_in_database(): table_name is empty.");
      return false;
    }

    Glib::ustring strQuery = "UPDATE \"" + table_name + "\"";
    strQuery += " SET \"" + field_name + "\" = " + field->sql(field_value);
    strQuery += " WHERE \"" + primary_key_field->get_name() + "\" = " + primary_key_field->sql(primary_key_value);
    Glib::RefPtr<Gnome::Gda::DataModel> datamodel = Query_execute(strQuery);  //TODO: Handle errors
    if(!datamodel)
    {
      g_warning("Box_Data::set_field_value_in_database(): UPDATE failed.");
      return false; //failed.
    }

    //Get-and-set values for lookup fields, if this field triggers those relationships:
    do_lookups(row, field_layout, field_value, primary_key_field, primary_key_value);

    //Update related fields, if this field is used in the relationship:
    refresh_related_fields(row, field_layout, field_value, primary_key_field, primary_key_value);

    //Calculate any dependent fields.
    //TODO: Make lookups part of this?
    //Prevent circular calculations during the recursive do_calculations:
    {
      //Recalculate any calculated fields that depend on this calculated field.
      //g_warning("Box_Data::set_field_value_in_database(): calling do_calculations");
      do_calculations(field_layout, primary_key_field, primary_key_value, !use_current_calculations);
    }
  }

  return true;
}

Document_Glom::type_mapLayoutGroupSequence Box_Data::get_data_layout_groups(const Glib::ustring& layout)
{
  Document_Glom::type_mapLayoutGroupSequence layout_groups;

  Document_Glom* document = dynamic_cast<Document_Glom*>(get_document());
  if(document)
  {
    if(!m_strTableName.empty())
    {
      //Get the layout information from the document:
      layout_groups = document->get_data_layout_groups_plus_new_fields(layout, m_strTableName);

      const Privileges table_privs = get_current_privs(m_strTableName);

      //Fill in the field information for the fields mentioned in the layout:
      for(Document_Glom::type_mapLayoutGroupSequence::iterator iterGroups = layout_groups.begin(); iterGroups != layout_groups.end(); ++iterGroups)
      {
        fill_layout_group_field_info(iterGroups->second, table_privs);
      }
    }
  }

  return layout_groups;
}

void Box_Data::fill_layout_group_field_info(const sharedptr<LayoutGroup>& group, const Privileges& table_privs)
{ 
  if(!group)
   return;

  const Document_Glom* document = get_document();

  LayoutGroup::type_map_items items = group->get_items();
  for(LayoutGroup::type_map_items::iterator iter = items.begin(); iter != items.end(); ++iter)
  {
    sharedptr<LayoutItem> item = iter->second;
    sharedptr<LayoutItem_Field> item_field = sharedptr<LayoutItem_Field>::cast_dynamic(item);
    if(item_field) //If is a field rather than some other layout item
    {

      if(item_field->get_has_relationship_name()) //If it's a field in a related table.
      {
        //Get the full field information:
        const Glib::ustring relationship_name = item_field->get_relationship_name();
        sharedptr<const Relationship> relationship = document->get_relationship(m_strTableName, relationship_name);
        if(relationship)
        {
          sharedptr<Field> field = get_fields_for_table_one_field(relationship->get_to_table(), item->get_name());
          if(field)
          {
            item_field->set_full_field_details(field);

            //TODO_Performance: Don't do this repeatedly for the same table.
            const Privileges privs = get_current_privs(relationship->get_to_table());
            item_field->m_priv_view = privs.m_view;
            item_field->m_priv_edit = privs.m_edit;
          }
        }
      }
      else
      {
        //Get the field info:
        sharedptr<Field> field = get_fields_for_table_one_field(m_strTableName, item_field->get_name());
        if(field)
        {
          item_field->set_full_field_details(field); //TODO_Performance: Just use this as the output arg?
          item_field->m_priv_view = table_privs.m_view;
          item_field->m_priv_edit = table_privs.m_edit;
        }
      }
    }
    else
    {
      sharedptr<LayoutGroup> item_group = sharedptr<LayoutGroup>::cast_dynamic(item);
      if(item_group) //If it is a group
      {
        //recurse, to fill the fields info in this group:
        fill_layout_group_field_info(item_group, table_privs);
      }
    }
  }
}

bool Box_Data::record_delete(const Gnome::Gda::Value& primary_key_value)
{

  sharedptr<Field> field_primary_key = get_field_primary_key();
  if(field_primary_key && !GlomConversions::value_is_empty(primary_key_value))
  {
    return Query_execute( "DELETE FROM \"" + m_strTableName + "\" WHERE \"" + m_strTableName + "\".\"" + field_primary_key->get_name() + "\" = " + field_primary_key->sql(primary_key_value) );
  }
  else
  {
    return false; //no primary key
  }
}

/*
bool Box_Data::get_field(const Glib::ustring& name, Field& field) const
{
g_warning("debug: Box_Data::get_field()");
  type_vecFields::const_iterator iterFind = std::find_if( m_TableFields.begin(), m_TableFields.end(), predicate_FieldHasName<LayoutItem_Field>(name) );
  if(iterFind != m_FieldsShown.end()) //If it was found:
  {
    field = iterFind->m_field;
    return true;
  }
  else
  {
    return false; //not found.
  }
}
*/

Glib::ustring Box_Data::build_sql_select(const Glib::ustring& table_name, const type_vecLayoutFields& fieldsToGet, const sharedptr<const Field>& primary_key_field, const Gnome::Gda::Value& primary_key_value)
{
  if(!GlomConversions::value_is_empty(primary_key_value)) //If there is a record to show:
  {
    const Glib::ustring where_clause = "\"" + table_name + "\".\"" + primary_key_field->get_name() + "\" = " + primary_key_field->sql(primary_key_value);
    return GlomUtils::build_sql_select_with_where_clause(table_name, fieldsToGet, where_clause);
  }

  return Glib::ustring();
}

/*
Glib::ustring Box_Data::build_sql_select_with_where_clause(const Glib::ustring& table_name, const type_vecLayoutFields& fieldsToGet, const Glib::ustring& where_clause)
{
  Glib::ustring result;

  Document_Glom* document = get_document();

  Glib::ustring sql_part_fields;

  typedef std::list<Relationship> type_list_relationships;
  type_list_relationships list_relationships;

  for(type_vecLayoutFields::const_iterator iter =  fieldsToGet.begin(); iter != fieldsToGet.end(); ++iter)
  {
    if(iter != fieldsToGet.begin())
      sql_part_fields += ", ";


    if(!iter->get_has_relationship_name())
    {
      sql_part_fields += ( table_name + "." );
    }
    else
    {
      Glib::ustring relationship_name = iter->get_relationship_name();
      Relationship relationship;
      bool test = document->get_relationship(table_name, relationship_name, relationship);
      if(test)
      {
        const Glib::ustring field_table_name = relationship->get_to_table();
        sql_part_fields += ( field_table_name + "." );

        //Add the relationship to the list:
        type_list_relationships::const_iterator iterFind = std::find_if(list_relationships.begin(), list_relationships.end(), predicate_FieldHasName<Relationship>( relationship_name ) );
        if(iterFind == list_relationships.end()) //If the table is not yet in the list:
          list_relationships.push_back(relationship);
      }
    }

    sql_part_fields += iter->get_name();
  }

  result =  "SELECT " + sql_part_fields +
    " FROM \"" + table_name + "\"";

  //LEFT OUTER JOIN will get the field values from the other tables, and give us our fields for this table even if there is no corresponding value in the other table.
  Glib::ustring sql_part_leftouterjoin; 
  for(type_list_relationships::const_iterator iter = list_relationships.begin(); iter != list_relationships.end(); ++iter)
  {
    const Relationship& relationship = *iter;
    sql_part_leftouterjoin += " LEFT OUTER JOIN " + relationship->get_to_table() +
      " ON (" + relationship->get_from_table() + "." + relationship->get_from_field() + " = " +
      relationship->get_to_table() + "." + relationship->get_to_field() +
      ")";
  }

  result += sql_part_leftouterjoin;

  if(!where_clause.empty())
    result += " WHERE " + where_clause;

  return result;
}
*/

bool Box_Data::get_related_record_exists(const sharedptr<const Relationship>& relationship, const sharedptr<const Field>& key_field, const Gnome::Gda::Value& key_value)
{
  bool result = false;

  //TODO_Performance: It's very possible that this is slow.
  //We don't care how many records there are, only whether there are more than zero.
  const Glib::ustring to_field = relationship->get_to_table() = relationship->get_to_field();

  //TODO_Performance: Is this the best way to just find out whether there is one record that meets this criteria?
  const Glib::ustring query = "SELECT \"" + to_field + "\" FROM \"" + relationship->get_to_table() + "\" WHERE \"" + to_field + "\" = " + key_field->sql(key_value);
  Glib::RefPtr<Gnome::Gda::DataModel> records = Query_execute(query);
  if(!records)
    handle_error();
  else
  {
    //Field contents:
    if(records->get_n_rows())
      result = true;
  }

  return result;
}

bool Box_Data::add_related_record_for_field(const sharedptr<const LayoutItem_Field>& layout_item_parent, const sharedptr<const Relationship>& relationship, const sharedptr<const Field>& primary_key_field, const Gnome::Gda::Value& primary_key_value_provided)
{
  Gnome::Gda::Value primary_key_value = primary_key_value_provided;

  const bool related_record_exists = get_related_record_exists(relationship, primary_key_field, primary_key_value);
  if(related_record_exists)
  {
    //No problem, the SQL command below will update this value in the related table.
  }
  else
  {
    //To store the entered data in the related field, we would first have to create a related record.
    if(!relationship->get_auto_create())
    {
      //Warn the user:
      //TODO: Make the field insensitive until it can receive data, so people never see this dialog.
      Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("Related Record Does Not Exist")), true);
      dialog.set_secondary_text(_("Data may not be entered into this related field, because the related record does not yet exist, and the relationship does not allow automatic creation of new related records."));
      dialog.set_transient_for(*get_app_window());
      dialog.run();

      //Clear the field again, discarding the entered data.
      set_entered_field_data(layout_item_parent, Gnome::Gda::Value());

      return false;
    }
    else
    {
      const bool key_is_auto_increment = primary_key_field->get_auto_increment();

      //If we would have to set an otherwise auto-increment key to add the record.
      if( key_is_auto_increment && !GlomConversions::value_is_empty(primary_key_value) )
      {
        //Warn the user:
        //TODO: Make the field insensitive until it can receive data, so people never see this dialog.
        Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("Related Record Cannot Be Created")), true);
        //TODO: This is a very complex error message:
        dialog.set_secondary_text(_("Data may not be entered into this related field, because the related record does not yet exist, and the key in the related record is auto-generated and therefore can not be created with the key value in this record."));
        dialog.set_transient_for(*get_app_window());
        dialog.run();

        //Clear the field again, discarding the entered data.
        set_entered_field_data(layout_item_parent, Gnome::Gda::Value());

        return false;
      }
      else
      {
        //TODO: Calculate values, and do lookups?

        //Create the related record:
        if(key_is_auto_increment)
        {
          primary_key_value = generate_next_auto_increment(relationship->get_to_table(), primary_key_field->get_name());

          //Generate the new key value;
        }

        const Glib::ustring strQuery = "INSERT INTO \"" + relationship->get_to_table() + "\" (\"" + primary_key_field->get_name() + "\") VALUES (" + primary_key_field->sql(primary_key_value) + ")";
        bool test = Query_execute(strQuery);
        if(test)
        {
          if(key_is_auto_increment)
          {
            //Set the key in the parent table
            sharedptr<LayoutItem_Field> item_from_key = sharedptr<LayoutItem_Field>::create();
            item_from_key->set_name(relationship->get_from_field());

            //Show the new from key in the parent table's layout:
            set_entered_field_data(item_from_key, primary_key_value);

            //Set it in the database too:
            sharedptr<Field> field_from_key = get_fields_for_table_one_field(relationship->get_from_table(), relationship->get_from_field()); //TODO_Performance.
            if(field_from_key)
            {
              sharedptr<Field> parent_primary_key_field = get_field_primary_key();
              if(!parent_primary_key_field)
              {
                g_warning("Box_Data::add_related_record_for_field(): get_field_primary_key() failed. table = %s", get_table_name().c_str());
                return false;
              }
              else
              {
                const Gnome::Gda::Value parent_primary_key_value = get_primary_key_value_selected();
                if(parent_primary_key_value.is_null())
                {
                  g_warning("Box_Data::add_related_record_for_field(): get_primary_key_value_selected() failed. table = %s", get_table_name().c_str());
                  return false;
                }
                else
                {
                  const Glib::ustring strQuery = "UPDATE \"" + relationship->get_from_table() + "\" SET \"" + relationship->get_from_field() + "\" = " + primary_key_field->sql(primary_key_value) +
                    " WHERE \"" + relationship->get_from_table() + "\".\"" + parent_primary_key_field->get_name() + "\" = " + parent_primary_key_field->sql(parent_primary_key_value);
                  const bool test = Query_execute(strQuery);
                  return test;
                }
              }
            }
          }
        }
      }
    }

  }

  return true;
}

void Box_Data::print_layout()
{
  Gtk::MessageDialog dialog("<b>Not implemented</b>", true);
  dialog.set_secondary_text("Sorry, this feature has not been implemented yet.");
  dialog.set_transient_for(*get_app_window());
  dialog.run();
}

Glib::ustring Box_Data::get_layout_name() const
{
  return m_layout_name;
}

bool Box_Data::confirm_delete_record()
{
  //Ask the user for confirmation:
  Gtk::MessageDialog dialog(Bakery::App_Gtk::util_bold_message(_("Delete record")), true, Gtk::MESSAGE_QUESTION, Gtk::BUTTONS_NONE);
  dialog.set_secondary_text(_("Are you sure that you would like to delete this record? The data in this record will then be permanently lost."));
  dialog.set_transient_for(*get_app_window());
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button(Gtk::Stock::DELETE, Gtk::RESPONSE_OK);

  const int response = dialog.run();
  return (response == Gtk::RESPONSE_OK);
}
