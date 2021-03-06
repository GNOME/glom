/* Glom
 *
 * Copyright (C) 2010 Murray Cumming
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

#include <glom/utility_widgets/cellrendererlist.h>
#include <glom/mode_data/datawidget/cellrenderer_dblist.h>
#include <glom/mode_data/datawidget/cellrenderer_buttontext.h>
#include <glom/utils_ui.h>
#include <glom/appwindow.h>
#include <gtkmm/cellrenderertext.h>
#include <libglom/data_structure/glomconversions.h>
#include <iostream>


namespace Glom
{

static void apply_formatting(Gtk::CellRenderer* renderer, const std::shared_ptr<const LayoutItem_WithFormatting>& layout_item)
{
  auto text_renderer = dynamic_cast<Gtk::CellRendererText*>(renderer);
  if(!text_renderer)
    return;

  //Use the text formatting:

  //Horizontal alignment:
  const Formatting::HorizontalAlignment alignment =
    layout_item->get_formatting_used_horizontal_alignment();
  const float x_align = (alignment == Formatting::HorizontalAlignment::LEFT ? 0.0 : 1.0);
  text_renderer->property_xalign() = x_align;

  const auto& formatting = layout_item->get_formatting_used();

  const auto font_desc = formatting.get_text_format_font();
  if(!font_desc.empty())
  text_renderer->property_font() = font_desc;

  const auto fg = formatting.get_text_format_color_foreground();
  if(!fg.empty())
    text_renderer->property_foreground() = fg;

  const auto bg = formatting.get_text_format_color_background();
  if(!bg.empty())
    text_renderer->property_background() = bg;
}

//This really needs to take the layout_item as a std::shared_ptr<>,
//because we might need to call set_layout_item(layout_item) on the newly-created cell.
//TODO: And it should be non-const too.
Gtk::CellRenderer* create_cell(const std::shared_ptr<const LayoutItem>& layout_item, const Glib::ustring& table_name, const std::shared_ptr<const Document>& document, guint fixed_cell_height)
{
  Gtk::CellRenderer* cell = nullptr;

  //Create the appropriate cellrenderer type:
  auto item_field = std::dynamic_pointer_cast<const LayoutItem_Field>(layout_item);
  if(item_field)
  {
    //Ignore hidden fields.
    //For instance, these are generally added to DbTreeModels when they would not otherwise contain the primary key,
    //so that the record can still be uniquely identified.
    if(item_field->get_hidden())
    {
      //std::cerr << G_STRFUNC << ": Returning 0 because the layout field is hidden. table_name=" << table_name << ", field name=" << item_field->get_name() << std::endl;
      return nullptr;
    }

    switch(item_field->get_glom_type())
    {
      case(Field::glom_field_type::BOOLEAN):
      {
         cell = Gtk::manage( new Gtk::CellRendererToggle() );

          break;
      }
      case(Field::glom_field_type::IMAGE):
      {
        cell = Gtk::manage( new Gtk::CellRendererPixbuf() );

        break;
      }
      default:
      {
        const auto& formatting = item_field->get_formatting_used();
        if(formatting.get_has_choices())
        {
          auto rendererList = Gtk::manage( new CellRendererDbList() );
          auto unconst = std::const_pointer_cast<LayoutItem>(layout_item); //TODO: Avoid this.
          rendererList->set_layout_item(unconst, table_name);
          bool as_radio_buttons = false; //Can't really be done in a list, so we ignore it.
          const auto restricted = formatting.get_choices_restricted(as_radio_buttons);
          rendererList->set_restrict_values_to_list(restricted);

          //Set the choices.
          //For related choices, that gets set when the value of a dependent field is set.
          if(formatting.get_has_custom_choices())
          {
            rendererList->set_choices_fixed( formatting.get_choices_custom(), restricted);
          }

          cell = rendererList;
        }
        else
          cell = Gtk::manage( new Gtk::CellRendererText() );

        break;
      }
    }
  }
  else
  {
    //Non-fields:

    auto item_image = std::dynamic_pointer_cast<const LayoutItem_Image>(layout_item);
    if(item_image)
    {
      auto pixbuf_renderer = Gtk::manage( new Gtk::CellRendererPixbuf() );

      const Glib::RefPtr<Gdk::Pixbuf> pixbuf = UiUtils::get_pixbuf_for_gda_value(item_image->m_image);
      if(pixbuf)
        pixbuf_renderer->property_pixbuf() = pixbuf;
      else
        pixbuf_renderer->property_icon_name() = "image-missing";

      cell = pixbuf_renderer;
    }
    else
    {
      auto item_text = std::dynamic_pointer_cast<const LayoutItem_Text>(layout_item);
      if(item_text)
      {
        auto pCellText = Gtk::manage( new Gtk::CellRendererText() );
        pCellText->property_text() = item_get_title(item_text);

        cell = pCellText;
      }
      else
      {
        auto item_button = std::dynamic_pointer_cast<const LayoutItem_Button>(layout_item);
        if(item_button)
        {
          auto pCellButton = Gtk::manage( new GlomCellRenderer_ButtonText() );
          pCellButton->property_text() = item_get_title_or_name(item_button);
          //pCellButton->set_fixed_width(50); //Otherwise it doesn't show up. TODO: Discover the width of the contents.

          cell = pCellButton;
        }
      }
    }
  }

  if(!cell)
  {
    std::cerr << G_STRFUNC << ": Returning 0 because no cell was created.\n";
    return nullptr;
  }

  //Use formatting:
  auto item_withformatting =
    std::dynamic_pointer_cast<const LayoutItem_WithFormatting>(layout_item);
  if(item_withformatting)
  {
    apply_formatting(cell, item_withformatting);
  }


  if(auto cell_text = dynamic_cast<Gtk::CellRendererText*>(cell))
  {
    //Use an ellipze to indicate excessive text,
    //so that similar values do not look equal,
    //and to avoid multi-line comments. TODO: Is there a better way to restrict the height? This doesn't actually truncate multilines anyway.
    cell_text->property_ellipsize() = Pango::ELLIPSIZE_END;

    //Restrict the height, to prevent multiline text cells,
    //and to allow TreeView performance optimisation:
    //TODO: Avoid specifying a width for the last column?
    int suitable_width = 0;
    cell_text->get_property("width", suitable_width);
    cell_text->set_fixed_size(suitable_width, fixed_cell_height);
  }

  //Choices:
  if(auto pCellRendererList = dynamic_cast<CellRendererList*>(cell)) //Used for custom choices:
  {
    pCellRendererList->remove_all_list_items();

    if(item_field && item_field->get_formatting_used().get_has_custom_choices())
    {
      //set_choices_fixed() needs this, for the numeric layout:
      //pCellRendererCombo->set_layout_item(get_layout_item()->clone(), table_name); //TODO_Performance: We only need this for the numerical format.
      for(const auto& value : item_field->get_formatting_used().get_choices_custom())
      {
        if(!value)
          continue;

        pCellRendererList->append_list_item( Conversions::get_text_for_gda_value(item_field->get_glom_type(), value->get_value(), item_field->get_formatting_used().m_numeric_format) );
      }
    }
  }
  else if(auto pCellRendererDbList = dynamic_cast<CellRendererDbList*>(cell)) //Used for related choices:
  {
    if(item_field && item_field->get_formatting_used().get_has_related_choices())
    {
      std::shared_ptr<const Relationship> choice_relationship;
      std::shared_ptr<const LayoutItem_Field> choice_field;
      std::shared_ptr<const LayoutGroup> choice_extras; //Ignored
      Formatting::type_list_sort_fields choice_sort_fields; //Ignored
      bool choice_show_all = false;
      item_field->get_formatting_used().get_choices_related(choice_relationship, choice_field, choice_extras, choice_sort_fields, choice_show_all);

      if(choice_relationship && choice_field)
      {
        const auto to_table = choice_relationship->get_to_table();

        //TODO: Update this when the relationship's field value changes:
        if(choice_show_all) //Otherwise it must change whenever the relationships's ID value changes.
        {
          pCellRendererDbList->set_choices_related(document, *item_field, Gnome::Gda::Value() /* TODO: Makes no sense */);
        }
      }
    }
  }

  return cell;
}

} //namespace Glom
