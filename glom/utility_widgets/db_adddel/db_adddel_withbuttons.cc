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

#include "db_adddel_withbuttons.h"
#include <glom/utils_ui.h>

namespace Glom
{

DbAddDel_WithButtons::DbAddDel_WithButtons()
: m_HBox(false, Utils::DEFAULT_SPACING_SMALL),
#ifndef GLOM_ENABLE_MAEMO
  m_Button_Add(Gtk::Stock::ADD),
  m_Button_Del(Gtk::Stock::DELETE),
  m_Button_Edit(Gtk::Stock::OPEN)
#else
  m_Button_Add(Gtk::Hildon::SIZE_FINGER_HEIGHT, Hildon::BUTTON_ARRANGEMENT_HORIZONTAL),
  m_Button_Del(Gtk::Hildon::SIZE_FINGER_HEIGHT, Hildon::BUTTON_ARRANGEMENT_HORIZONTAL),
  m_Button_Edit(Gtk::Hildon::SIZE_FINGER_HEIGHT, Hildon::BUTTON_ARRANGEMENT_HORIZONTAL)
#endif
{
  m_HBox.set_spacing(Utils::DEFAULT_SPACING_SMALL);

  setup_buttons();
  pack_start(m_HBox, Gtk::PACK_SHRINK);

  //Link buttons to handlers:
  m_Button_Add.signal_clicked().connect(sigc::mem_fun(*this, &DbAddDel_WithButtons::on_button_add));
  m_Button_Del.signal_clicked().connect(sigc::mem_fun(*this, &DbAddDel_WithButtons::on_button_del));
  m_Button_Edit.signal_clicked().connect(sigc::mem_fun(*this, &DbAddDel_WithButtons::on_button_edit));

  m_HBox.pack_end(m_Button_Edit, Gtk::PACK_SHRINK);
  m_HBox.pack_end(m_Button_Del, Gtk::PACK_SHRINK);
  m_HBox.pack_end(m_Button_Add, Gtk::PACK_SHRINK);

#ifdef GLOM_ENABLE_MAEMO
  //Use smaller icon-only buttons for these infrequently-clicked buttons,
  //to save screen space.
  
  //Note that icons of size Gtk::ICON_SIZE_MENU are smaller, 
  //but it seems impossible to have Hildon::Buttons smaller than Gtk::Hildon::SIZE_FINGER_HEIGHT.
  Gtk::Image* image = Gtk::manage(new Gtk::Image(Gtk::Stock::ADD, Gtk::ICON_SIZE_SMALL_TOOLBAR));
  m_Button_Add.set_image(*image);
  
  image = Gtk::manage(new Gtk::Image(Gtk::Stock::DELETE, Gtk::ICON_SIZE_SMALL_TOOLBAR));
  m_Button_Del.set_image(*image);
  
  image = Gtk::manage(new Gtk::Image(Gtk::Stock::OPEN, Gtk::ICON_SIZE_SMALL_TOOLBAR));
  m_Button_Edit.set_image(*image);
#endif //GLOM_ENABLE_MAEMO

  setup_buttons();
}

DbAddDel_WithButtons::~DbAddDel_WithButtons()
{
}

void DbAddDel_WithButtons::on_button_add()
{
#ifndef GLOM_ENABLE_MAEMO
  on_MenuPopup_activate_Add();
#endif
}

void DbAddDel_WithButtons::on_button_del()
{
#ifndef GLOM_ENABLE_MAEMO
  on_MenuPopup_activate_Delete();
#endif
}

void DbAddDel_WithButtons::on_button_edit()
{
  do_user_requested_edit();
}

void DbAddDel_WithButtons::set_allow_add(bool val)
{
  DbAddDel::set_allow_add(val);

  setup_buttons();
}

void DbAddDel_WithButtons::set_allow_delete(bool val)
{
  DbAddDel::set_allow_delete(val);

  setup_buttons();
}

void DbAddDel_WithButtons::set_allow_user_actions(bool bVal)
{
  DbAddDel::set_allow_user_actions(bVal);

  setup_buttons();

  //Recreate popup menu with correct items:
  setup_menu();
}

void DbAddDel_WithButtons::setup_buttons()
{
  const bool allow_edit = get_allow_user_actions() && get_allow_view_details();
  const bool allow_del = get_allow_user_actions() && m_allow_delete;
  const bool allow_add = get_allow_user_actions() && m_allow_add;
 
  m_Button_Edit.show();
  m_Button_Del.show();
  m_Button_Add.show();
  
  m_Button_Edit.set_property("visible", allow_edit);
  m_Button_Del.set_property("visible", allow_del);
  m_Button_Add.set_property("visible", allow_add);
  
  if(!m_open_button_title.empty())
    m_Button_Edit.set_label(m_open_button_title);
  
  m_HBox.show();
}

// TODO_maemo: Why is this show_all_vfunc, and not on_show()? Where is the
// difference? If this was on_show we could just connect to signal_show()
// when vfuncs and/or default signal handlers are not available.
void DbAddDel_WithButtons::show_all_vfunc()
{
  //Call the base class:
  Gtk::VBox::show_all_vfunc();
  
  //Hide some stuff:
  setup_buttons();
}

void DbAddDel_WithButtons::set_allow_view_details(bool val)
{
  DbAddDel::set_allow_view_details(val);
  
  setup_buttons();
}


} //namespace Glom
