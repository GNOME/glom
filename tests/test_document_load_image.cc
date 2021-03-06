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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include "tests/test_utils_images.h"
#include <libglom/document/document.h>
#include <libglom/init.h>
#include <libglom/db_utils.h>
#include <giomm/file.h>
#include <glibmm/convert.h>
#include <glibmm/miscutils.h>

#include <iostream>

int main()
{
  Glom::libglom_init();

  // Get a URI for a test file:
  Glib::ustring uri;

  try
  {
    const auto path =
       Glib::build_filename(GLOM_DOCDIR_EXAMPLES_NOTINSTALLED,
   "example_project_manager.glom");
    uri = Glib::filename_to_uri(path);
  }
  catch(const Glib::ConvertError& ex)
  {
    std::cerr << G_STRFUNC << ": " << ex.what();
    return EXIT_FAILURE;
  }

  //std::cout << "URI=" << uri << std::endl;


  // Load the document:
  auto document = std::make_shared<Glom::Document>();
  document->set_file_uri(uri);
  int failure_code = 0;
  const auto test = document->load(failure_code);
  //std::cout << "Document load result=" << test << std::endl;

  if(!test)
  {
    std::cerr << G_STRFUNC << ": Document::load() failed with failure_code=" << failure_code << std::endl;
    return EXIT_FAILURE;
  }

  //Test some known details:
  g_assert(document->get_is_example_file());
  g_assert(document->get_database_title_original() == "Project Manager Example");

  //Check a layout:
  const auto groups =
    document->get_data_layout_groups("details", "projects");
  g_assert(groups.size() == 3);
  const auto group =
    groups[0];
  g_assert(group);
  g_assert(group->get_name() == "overview");

  const auto items =
    group->get_items();
  //std::cout << "size: " << items.size() << std::endl;
  g_assert(items.size() == 3);
  auto item = items[2];
  g_assert(item);
  auto image_item =
    std::dynamic_pointer_cast<const Glom::LayoutItem_Image>(item);
  g_assert(image_item);

  const auto value = image_item->get_image();
  g_assert(check_value_is_an_image(value));

  Glom::libglom_deinit();

  return EXIT_SUCCESS;
}
