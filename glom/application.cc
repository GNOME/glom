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

#include "application.h"
#include "config.h" //For VERSION.
#include <cstdio>

App_Glom::App_Glom(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& refGlade)
: type_base(cobject, "Glom"),
  Gtk::Window(cobject), //It's a virtual base class, so we have to call the specific constructor to prevent the default constructor from being called.
  m_pBoxTop(0),
  m_pFrame(0),
  m_pStatus(0)
{
  //Load widgets from glade file:
  refGlade->get_widget("bakery_vbox", m_pBoxTop);
  refGlade->get_widget_derived("vbox_frame", m_pFrame); //This one is derived. There's a lot happening here.
  refGlade->get_widget("label_hint", m_pStatus);

  AppState* pAppState = AppState::get_instance();
  if(pAppState)
  {
    pAppState->signal_userlevel_changed().connect( sigc::mem_fun(*this, &App_Glom::on_userlevel_changed) );
  }
  
  add_mime_type("application/x-glom"); //TODO: make this actually work - we need to register it properly.
  
  show_all_children();
}

App_Glom::~App_Glom()
{

}

void App_Glom::init()
{
  type_vecStrings vecAuthors;
  vecAuthors.push_back("Murray Cumming <murrayc@murrayc.com>");
  set_about_information(VERSION, vecAuthors, gettext("(C) 2000-2004 Murray Cumming"), gettext("A Database GUI"));

  type_base::init(); //calls init_menus() and init_toolbars()

  //m_pFrame->set_shadow_type(Gtk::SHADOW_IN);

  Document_Glom* pDocument = static_cast<Document_Glom*>(get_document());
  if(pDocument->get_connection_database().empty()) //If it is a new (default) document.
  {
    offer_new_or_existing();
  }
  
  //show_all();
}

void App_Glom::init_layout()
{
  //We override this method so that we can put everything in the vbox from the glade file, instead of the vbox from App_Gtk.
  
  //Add menu bar at the top:
  //These were defined in init_uimanager().
  Gtk::MenuBar* pMenuBar = static_cast<Gtk::MenuBar*>(m_refUIManager->get_widget("/Bakery_MainMenu"));
  m_pBoxTop->pack_start(*pMenuBar, Gtk::PACK_SHRINK);

  Gtk::Toolbar* pToolBar = static_cast<Gtk::Toolbar*>(m_refUIManager->get_widget("/Bakery_ToolBar"));
  m_HandleBox_Toolbar.add(*pToolBar);
  m_HandleBox_Toolbar.show();

  add_accel_group(m_refUIManager->get_accel_group());

  m_pBoxTop->pack_start(m_HandleBox_Toolbar, Gtk::PACK_SHRINK);

  //Add placeholder, to be used by add():
  //m_pBoxTop->pack_start(m_VBox_PlaceHolder);
  //m_VBox_PlaceHolder.show();
}

void App_Glom::init_menus()
{
  init_menus_file();
  init_menus_edit();

  //Build actions:
  m_refActionGroup_Others = Gtk::ActionGroup::create("GlomOthersActions");

  //We save some action in m_listDeveloperActions - these will all be enabled/disabled when the userlevel changes.
  m_listDeveloperActions.push_back(m_action_save);
  m_listDeveloperActions.push_back(m_action_saveas);
  
  //"Navigate" menu:
  m_refActionGroup_Others->add( Gtk::Action::create("Glom_Menu_Navigate", gettext("_Navigate")) );

  Glib::RefPtr<Gtk::Action> action = Gtk::Action::create("GlomAction_Menu_Navigate_Database", gettext("_Database"));
  m_listDeveloperActions.push_back(action);
  m_refActionGroup_Others->add(action,
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_Navigate_Database) );


  action = Gtk::Action::create("GlomAction_Menu_Navigate_Table", gettext("_Table"));
  m_listWithDatabaseActions.push_back(action); //Remember it so we can disable/enable it when a database has been selected.
  m_refActionGroup_Others->add(action,
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_Navigate_Table) );

  //"UserLevel" menu:
  m_refActionGroup_Others->add(Gtk::Action::create("Glom_Menu_UserLevel", gettext("_User Level")));
  Gtk::RadioAction::Group group_userlevel;
  m_refActionGroup_Others->add(Gtk::RadioAction::create(group_userlevel, "GlomAction_Menu_UserLevel_Developer", gettext("_Developer")),
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_UserLevel_Developer) );
  m_refActionGroup_Others->add(Gtk::RadioAction::create(group_userlevel, "GlomAction_Menu_UserLevel_Operator", gettext("_Operator")),
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_UserLevel_Operator) );

  //"Mode" menu:
  action =  Gtk::Action::create("Glom_Menu_Mode", gettext("_Mode"));
  m_listWithDatabaseActions.push_back(action);
  m_refActionGroup_Others->add(action);
  Gtk::RadioAction::Group group_mode;

  action = Gtk::RadioAction::create(group_mode, "GlomAction_Menu_Mode_Design", gettext("_Design"));
  m_listDeveloperActions.push_back(action);
  m_refActionGroup_Others->add(action,
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_Mode_Design) );

  //We remember this action, so that it can be explicitly activated later.
  m_action_mode_data = Gtk::RadioAction::create(group_mode, "GlomAction_Menu_Mode_Data", gettext("D_ata"));
  m_refActionGroup_Others->add(m_action_mode_data,
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_Mode_Data) );
  m_refActionGroup_Others->add( Gtk::RadioAction::create(group_mode, "GlomAction_Menu_Mode_Find", gettext("_Find")),  Gtk::AccelKey("<control>F"),
                        sigc::mem_fun(*m_pFrame, &Frame_Glom::on_menu_Mode_Find) );

  action = Gtk::Action::create("Glom_Menu_Developer", gettext("_Developer"));
  m_listDeveloperActions.push_back(action);
   m_refActionGroup_Others->add(action);

  m_refUIManager->insert_action_group(m_refActionGroup_Others);

  //Build part of the menu structure, to be merged in by using the "Bakery_MenuPH_Others" placeholder:
  static const Glib::ustring ui_description =
    "<ui>"
    "  <menubar name='Bakery_MainMenu'>"
    "    <placeholder name='Bakery_MenuPH_Others'>"
    "      <menu action='Glom_Menu_Navigate'>"
    "        <menuitem action='GlomAction_Menu_Navigate_Database' />"
    "        <menuitem action='GlomAction_Menu_Navigate_Table' />"
    "     </menu>"
    "      <menu action='Glom_Menu_UserLevel'>"
    "        <menuitem action='GlomAction_Menu_UserLevel_Developer' />"
    "        <menuitem action='GlomAction_Menu_UserLevel_Operator' />"
    "      </menu>"
    "      <menu action='Glom_Menu_Mode'>"
    "        <menuitem action='GlomAction_Menu_Mode_Design' />"
    "        <menuitem action='GlomAction_Menu_Mode_Data' />"
    "        <menuitem action='GlomAction_Menu_Mode_Find' />"
    "      </menu>"
     "      <menu action='Glom_Menu_Developer'>"
    "         <placeholder name='Glom_Menu_Developer_PH' />"
    "      </menu>"
    "    </placeholder>"
    "  </menubar>"
    "</ui>";

  //Add menu:
  add_ui_from_string(ui_description);

  init_menus_help();

  //Disable/Enable actions, depending on userlevel:
  AppState* pAppState = AppState::get_instance();
  if(pAppState)
    pAppState->emit_userlevel_changed();

  on_database_selected(false);
}


Bakery::App* App_Glom::new_instance() //Override
{
  Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(GLOM_GLADEDIR "glom.glade", "window_main");
  App_Glom* pApp_Glom = 0;
  refXml->get_widget_derived("window_main", pApp_Glom);

  return pApp_Glom;
}

void App_Glom::init_create_document()
{
  if(!m_pDocument)
  {
    m_pDocument = new Document_Glom();

    //Tell document about view:
    m_pDocument->set_view(m_pFrame);

    //Tell view about document:
    //(This calls set_document() in the child views too.)
    m_pFrame->set_document(static_cast<Document_Glom*>(m_pDocument));
  }

  type_base::init_create_document(); //Sets window title. Doesn't recreate doc.
}

void App_Glom::on_document_load()
{
  //Link to the database described in the document.
  //Need to ask user for user/password:
  //m_pFrame->load_from_document();
  Document_Glom* pDocument = static_cast<Document_Glom*>(get_document());
  if(pDocument->get_connection_database().empty()) //If it is a new (default) document.
  {
    //offer_new_or_existing();
  }
  else
  {
    //Read the connection information from the document:
    ConnectionPool* connection_pool = ConnectionPool::get_instance();
    if(connection_pool)
    {
      //Set the connection details in the ConnectionPool singleton.
      //The ConnectionPool will now use these every time it tries to connect.
      connection_pool->set_host(pDocument->get_connection_server());
      connection_pool->set_user(pDocument->get_connection_user());

      connection_pool->set_ready_to_connect(); //Box_DB::connect_to_server() will now attempt the connection-> Shared instances of m_Connection will also be usable.
    }

    m_pFrame->do_menu_Navigate_Database(false); //false = don't show list, just open the current database after connecting.
  }
}

void App_Glom::statusbar_set_text(const Glib::ustring& strText)
{
  m_pStatus->set_text(strText);
}

void App_Glom::statusbar_clear()
{
  statusbar_set_text("");
}

void App_Glom::on_userlevel_changed(AppState::userlevels userlevel)
{
  //Disable/Enable developer actions:
  for(type_listActions::iterator iter = m_listDeveloperActions.begin(); iter != m_listDeveloperActions.end(); ++iter)
  {
    Glib::RefPtr<Gtk::Action> action = *iter;
     action->set_sensitive ( userlevel == AppState::USERLEVEL_DEVELOPER );
  }
}

void App_Glom::on_database_selected(bool database_selected)
{
  //Disable/Enable developer actions:
  for(type_listActions::iterator iter = m_listWithDatabaseActions.begin(); iter != m_listWithDatabaseActions.end(); ++iter)
  {
    Glib::RefPtr<Gtk::Action> action = *iter;
     action->set_sensitive ( database_selected );
  }
}

Glib::RefPtr<Gtk::UIManager> App_Glom::get_ui_manager()
{
  return m_refUIManager;
}

void App_Glom::offer_new_or_existing()
{
  //Offer to load an existing document, or start a new one.
  Glib::RefPtr<Gnome::Glade::Xml> refXml = Gnome::Glade::Xml::create(GLOM_GLADEDIR "glom.glade", "dialog_existing_or_new");
  Gtk::Dialog* dialog = 0;
  refXml->get_widget("dialog_existing_or_new", dialog);
  dialog->set_transient_for(*this);
  
  guint response_id = dialog->run();
  delete dialog;
  dialog = 0;
  
  if(response_id ==1) //Open
  {
    on_menu_file_open();
  }
  else if(response_id == 2) //New
  {
    //There is already an empty document, but this allows the user to fill it by connecting and creating tables:
    m_pFrame->do_menu_Navigate_Database();
  }
  else
  {
    //Do nothing. TODO: Do something?
  }
}

void App_Glom::set_mode_data()
{
  m_action_mode_data->activate();
}



