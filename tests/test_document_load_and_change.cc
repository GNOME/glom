/* Glom
 *
 * Copyright (C) 2010 Openismus GmbH
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
71 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <libglom/document/document.h>
#include <libglom/init.h>
#include <giomm/file.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>

#include <iostream>

template<typename T_Container>
bool contains(const T_Container& container, const Glib::ustring& name)
{
  typename T_Container::const_iterator iter =
    std::find(container.begin(), container.end(), name);
  return iter != container.end();
}

template<typename T_Container>
bool contains_named(const T_Container& container, const Glib::ustring& name)
{
  typedef typename T_Container::value_type::object_type type_item;
  typename T_Container::const_iterator iter =
    std::find_if(container.begin(), container.end(),
      Glom::predicate_FieldHasName<type_item>(name));
  return iter != container.end();
}

int main()
{
  Glom::libglom_init();

  // Get a URI for a test file:
  Glib::ustring uri;

  try
  {
    const std::string path =
       Glib::build_filename(GLOM_DOCDIR_EXAMPLES_NOTINSTALLED,
         "example_music_collection.glom");
    uri = Glib::filename_to_uri(path);
  }
  catch(const Glib::ConvertError& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what();
    return EXIT_FAILURE;
  }

  //std::cout << "URI=" << uri << std::endl;


  // Load the document:
  Glom::Document document;
  document.set_file_uri(uri);
  int failure_code = 0;
  const bool test = document.load(failure_code);
  //std::cout << "Document load result=" << test << std::endl;

  if(!test)
  {
    std::cerr << "Document::load() failed with failure_code=" << failure_code << std::endl;
    return EXIT_FAILURE;
  }

  //Prevent these test changes from being saved back to the example file:
  document.set_allow_autosave(false);

  //Change a field name throughout the document:
  const Glib::ustring table_name = "songs";
  const Glib::ustring field_name_original = "song_id";
  const Glib::ustring field_name_new = "newfieldname";
  document.change_field_name(table_name, field_name_original, field_name_new);

  //Check that the original field name is not known to the document:
  if(document.get_field(table_name, field_name_original))
  {
    std::cerr << "Failure: The document should have forgotten about the original field name." << std::endl;
    return false;
  }

  //Check that the new field name is known to the document:
  if(!(document.get_field(table_name, field_name_new)))
  {
    std::cerr << "Failure: The document does not know about the new field name." << std::endl;
    return false;
  }

  //Check that the original field name is no longer used in the relationship:
  const Glom::sharedptr<const Glom::Relationship> relationship = document.get_relationship("songs", "album");
  if(!relationship)
  {
    std::cerr << "Failure: The relationship could not be found in the document." << std::endl;
    return false;
  }

  if(relationship->get_from_field() == field_name_original)
  {
    std::cerr << "Failure: The relationship still uses the original field name." << std::endl;
    return false;
  }

  //Check that the original field name is no longer used on a layout:
  const std::vector<Glib::ustring> table_names = document.get_table_names();
  for(std::vector<Glib::ustring>::const_iterator iter = table_names.begin(); iter != table_names.end(); ++iter)
  {
    const Glib::ustring table_name = *iter;
    const Glom::Document::type_list_layout_groups groups = 
      document.get_data_layout_groups("details", table_name);

    for(Glom::Document::type_list_layout_groups::const_iterator iter = groups.begin(); iter != groups.end(); ++iter)
    {
      const Glom::sharedptr<Glom::LayoutGroup> group = *iter;
      if(group->has_field(field_name_original))
      {
        std::cerr << "Failure: The field is still used on a layout for table: " << table_name << std::endl;
        return false;
      }
    }
  }

  Glom::libglom_deinit();

  return EXIT_SUCCESS;
}
