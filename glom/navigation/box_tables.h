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

#ifndef BOX_TABLES_H
#define BOX_TABLES_H

#include "../box_db.h"

/**
  *@author Murray Cumming
  */

class Box_Tables : public Box_DB
{
public:
  Box_Tables(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade);
  virtual ~Box_Tables();

protected:
  virtual void fill_from_database(); //override

  //Signal handlers:
  virtual void on_AddDel_Add(guint row);
  virtual void on_AddDel_Delete(guint rowStart, guint rowEnd);
  virtual void on_AddDel_Edit(guint row);

  virtual void on_show_hidden_toggled();

  Gtk::Label* m_pLabelFrameTitle;
  Gtk::CheckButton* m_pCheckButtonShowHidden;
  guint m_colHidden;
  guint m_colTitle;

  mutable AddDel_WithButtons m_AddDel; //mutable because its get_ methods aren't const.

};

#endif //BOX_TABLES_H

