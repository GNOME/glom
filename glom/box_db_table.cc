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

#include "box_db_table.h"
#include "data_structure/glomconversions.h"
#include "glom_python.h"
#include <sstream>

Box_DB_Table::Box_DB_Table()
{
}

Box_DB_Table::Box_DB_Table(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade)
: Box_DB(cobject, refGlade)
{
}

Box_DB_Table::~Box_DB_Table()
{
}

Glib::ustring Box_DB_Table::get_table_name()
{
  return m_strTableName;
}

void Box_DB_Table::init_db_details(const Glib::ustring& strDatabaseName, const Glib::ustring& strTableName, const Glib::ustring& strWhereClause /* = "" */)
{
  m_strDatabaseName = strDatabaseName;
  m_strTableName = strTableName;
  m_strWhereClause = strWhereClause;

  fill_fields();
  fill_from_database();
}

Glib::ustring Box_DB_Table::get_primarykey_name()
{
  Glib::ustring strResult;

  guint uiCol = 0;
  bool bPresent = get_field_primary_key(uiCol);

  if(bPresent)
  {
    strResult = m_Fields[uiCol].get_name();
  }
  else
  {
    std::cout << "Box_DB_Table::get_primarykey_name(): not found in " << m_Fields.size() << " fields." << std::endl;
  }

  return strResult;
}

void Box_DB_Table::fill_fields()
{

}

bool Box_DB_Table::record_delete(const Gnome::Gda::Value& primary_key_value)
{
  Field field_primary_key;
  bool test = get_field_primary_key(field_primary_key);
  if(test && !GlomConversions::value_is_empty(primary_key_value))
  {
    return Query_execute( "DELETE FROM " + m_strTableName + " WHERE " + m_strTableName + "." + field_primary_key.get_name() + " = " + field_primary_key.sql(primary_key_value) );
  }
  else
  {
    return false; //no primary key
  }
}

Gnome::Gda::Value Box_DB_Table::get_entered_field_data(const Field& /* field */) const
{
  //Override this to use Field::set_data() too.

  return Gnome::Gda::Value(); //null
}

void Box_DB_Table::set_entered_field_data(const Field& /* field */, const Gnome::Gda::Value& /* value */)
{
  //Override this.
}

bool Box_DB_Table::get_field(const Glib::ustring& name, Field& field) const
{
  type_vecFields::const_iterator iterFind = std::find_if( m_Fields.begin(), m_Fields.end(), predicate_FieldHasName<Field>(name) );
  if(iterFind != m_Fields.end()) //If it was found:
  {
    field = *iterFind;
    return true;
  }
  else
  {
    return false; //not found.
  }
}

bool Box_DB_Table::get_field_primary_key(guint& field_column) const
{
  return get_field_primary_key(m_Fields, field_column);
}

//static:
bool Box_DB_Table::get_field_primary_key(const type_vecFields& fields, guint& field_column)
{
  //Initialize input parameter:
  field_column = 0;
  
  //TODO_performance: Cache the primary key?
  guint col = 0;
  guint cols_count = fields.size();
  while(col < cols_count)
  {
    if(fields[col].get_field_info().get_primary_key())
    {
      field_column = col;
      return true;
    }
    else
    {
      ++col;
    }
  }

  return false; //Not found.
}

bool Box_DB_Table::get_field_primary_key(Field& field) const
{
 return get_field_primary_key(m_Fields, field);
}

//static:
bool Box_DB_Table::get_field_primary_key(const type_vecFields& fields, Field& field)
{
  //Initialize input parameter:
  field = Field();

  //TODO_performance: Cache the primary key?
  guint col = 0;
  guint cols_count = fields.size();
  while(col < cols_count)
  {
    if(fields[col].get_field_info().get_primary_key())
    {
      field = fields[col];
      return true;
    }
    else
    {
      ++col;
    }
  }

  return false; //Not found.
}

bool Box_DB_Table::get_field_index(const Glib::ustring& name, guint& field_index) const
{
  //Initialize input parameter:
  field_index = 0;

  //TODO_performance: Cache the primary key?
  guint col = 0;
  guint cols_count = m_Fields.size();
  while(col < cols_count)
  {
    if(m_Fields[col].get_name() == name)
    {
      field_index = col;
      return true;
    }
    else
      ++col;
  }

  return false; //Not found.
}

unsigned long Box_DB_Table::get_last_auto_increment_value(const Glib::RefPtr<Gnome::Gda::DataModel>& data_model, const Glib::ustring /* field_name */)
{
  sharedptr<SharedConnection> sharedconnection = connect_to_server();
  if(sharedconnection)
  {
    Glib::RefPtr<Gnome::Gda::Connection> connection = sharedconnection->get_gda_connection();
    Glib::ustring id = connection->get_last_insert_id(data_model);

    //Convert it to a numeric type:
    std::stringstream stream;
    stream << id;
    unsigned long id_numeric = 0;
    stream >> id_numeric;

    return id_numeric;
  }
  else
    return 0;
}

bool Box_DB_Table::get_fields_for_table_one_field(const Glib::ustring& table_name, const Glib::ustring& field_name, Field& field) const
{
  //Initialize output parameter:
  Field result = Field();

  type_vecFields fields =  get_fields_for_table(table_name);
  type_vecFields::iterator iter = std::find_if(fields.begin(), fields.end(), predicate_FieldHasName<Field>(field_name));
  if(iter != fields.end()) //TODO: Handle error?
  {
    field = *iter;
    return true;
  }

  return false;
}



//static:
Box_DB_Table::type_vecFields Box_DB_Table::get_fields_for_datamodel(const Glib::RefPtr<Gnome::Gda::DataModel>& data_model)
{
  type_vecFields result;

  int columns =  data_model->get_n_columns();
  for(int i = 0; i < columns; ++i)
  {
    Gnome::Gda::FieldAttributes fieldinfo = data_model->describe_column(i);
    Field field;
    field.set_field_info(fieldinfo); //TODO: Get glom-specific information from document?
    result.push_back( field );
  }

  return result;
}

Glib::ustring Box_DB_Table::postgres_get_field_definition_for_sql(const Gnome::Gda::FieldAttributes& field_info)
{
  Glib::ustring strResult;

  //Type
  Glib::ustring strType = "unknowntype";
  
  //Postgres has a special "serial" datatype. (MySQL uses a numeric type, and has an extra "AUTO_INCREMENT" command)
  if(false) //disabled for now - see generate_next_auto_increment() //field_info.get_auto_increment())
  {
    strType = "serial";
  }
  else
  {
    ConnectionPool* pConnectionPool = ConnectionPool::get_instance();
    if(pConnectionPool)
    {
      const FieldTypes* pFieldTypes = pConnectionPool->get_field_types();
      if(pFieldTypes)
      {
        const Gnome::Gda::ValueType fieldType = field_info.get_gdatype();
        strType = pFieldTypes->get_string_name_for_gdavaluetype(fieldType);
      }
    }
  }
  
  strResult += strType;
  
   /*
  //Optinal type details: (M, D), UNSIGNED
  Field::enumTypeOptionals optionals = fieldType.get_TypeOptionals();
  if(optionals != Field::TYPE_OPTIONALS_None)
  {
  Glib::ustring strOptionals;

  char pchM[10] = {0};
  sprintf(pchM, "%d", fieldType.get_MaxLength());
    Glib::ustring strM(pchM);


  if(optionals == Field::TYPE_OPTIONALS_M_D)
  {
    if( (fieldType.get_MaxLength() != 0) && ( fieldType.get_DecimalsCount() != 0) ) //0 here means use default, so don't specify.
    {
      char pchD[10] = {0};
      sprintf(pchD, "%d", fieldType.get_DecimalsCount());
      Glib::ustring strD(pchD);

      strOptionals = "(" + strM + "," + strD + ")";
    }

  }
  else if(optionals == Field::TYPE_OPTIONALS_M_S)
  {
    if( fieldType.get_MaxLength() != 0 ) //0 here means use default, so don't specify.
      strOptionals = "(" + strM + ")";

      if(!(fieldType.get_Signed()))
        strOptionals += " UNSIGNED";
  }
  else if(optionals == Field::TYPE_OPTIONALS_M)
  {
      if( fieldType.get_MaxLength() != 0 ) //0 here means use default, so don't specify.
      strOptionals = "(" + strM + ")";
  }

    if(strOptionals.size())
      strResult += (" " + strOptionals);
  }
  */

  //Unique:
  if(field_info.get_unique_key())
    strResult += " UNIQUE";
      
  /* Posgres needs us to do this separately
  //Not Null:
  if(!(field_info.get_allow_null()))
    strResult += " NOT NULL";
  */
    
  //Default:
  Gnome::Gda::Value valueDefault = field_info.get_default_value();
  const Glib::ustring& strDefault =  valueDefault.to_string();
  if(!strDefault.empty() && strDefault != "NULL")
    strResult += " DEFAULT " + strDefault; //TODO: Quote/Escape it if necessary.
      
  //Primary Key:
  if(field_info.get_primary_key())
    strResult += " PRIMARY KEY";
      
  return strResult;
}



