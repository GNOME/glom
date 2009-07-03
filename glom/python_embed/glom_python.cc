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

//We need to include this before anything else, to avoid redefinitions:
#include <libglom/python_embed/py_glom_record.h>
#include <libglom/python_embed/pygdavalue_conversions.h>

#define NO_IMPORT_PYGTK //To avoid a multiple definition in pygtk.
#include <pygtk/pygtk.h> //For the PyGObject and PyGBoxed struct definitions.

#include <Python.h>
#include <compile.h> /* for the PyCodeObject */
#include <eval.h> /* for PyEval_EvalCode */

#include "glom_python.h"
#include <libglom/data_structure/glomconversions.h>
#include <list>
#include <glib.h> //For g_warning().
#include <glibmm/i18n.h>

#include <gtkmm/messagedialog.h>

namespace Glom
{

std::list<Glib::ustring> ustring_tokenize(const Glib::ustring& msg, const Glib::ustring& separators, int maxParts)
{
  std::list<Glib::ustring> result;
  Glib::ustring str = msg;
  bool nocount = false;
  if(maxParts == -1)
    nocount = true;

  int count = 0;

  while(str.find(separators) != Glib::ustring::npos && (nocount? true : count!=maxParts))
  {
    unsigned int pos = str.find(separators);
    Glib::ustring tmp = str.substr(0, pos);
    str = str.erase(0, pos + separators.size());
    result.push_back(tmp);
    count++;
  }
  result.push_back(str);

  return result;
}

// Use this for errors not (directly) caused by the user
void HandlePythonError()
{
  if(PyErr_Occurred())
    PyErr_Print();
}

// Show python coding errors of the user
void ShowTrace()
{
  // Python equivilant:
  // import traceback, sys
  // return "".join(traceback.format_exception(sys.exc_type,
  //    sys.exc_value, sys.exc_traceback))

  PyObject *type, *value, *traceback;

  PyErr_Fetch(&type, &value, &traceback);
  
  if(!traceback)
  {
    std::cerr << "traceback = 0" << std::endl;
  }

  PyObject *tracebackModule = PyImport_ImportModule((char*)"traceback");
  gchar* chrRetval = 0;
  if(tracebackModule != NULL)
  {
      PyObject* tbList = PyObject_CallMethod(
          tracebackModule,
          (char*)"format_exception",
          (char*)"OOO",
          type,
          value == NULL ? Py_None : value,
          traceback == NULL ? Py_None : traceback);
      
      if(!tbList)
      {
        std::cerr << "format_exception failed" << std::endl;
        return;
      }

      PyObject* emptyString = PyString_FromString("");
      PyObject* strRetval = PyObject_CallMethod(emptyString, (char*)"join",
            (char*)"O", tbList);
      if(strRetval)
        chrRetval = g_strdup(PyString_AsString(strRetval));
    
      Py_DECREF(tbList);
      Py_DECREF(emptyString);
      Py_DECREF(strRetval);
      Py_DECREF(tracebackModule);
    }
    else
    {
        std::cerr << "Unable to import traceback module." << std::endl;
        
    }

    Py_DECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
  
  if(chrRetval)
  {
    Glib::ustring message = _("Python Error: \n");
    message += chrRetval;
    Gtk::MessageDialog dialog(message, false, Gtk::MESSAGE_ERROR);
    dialog.run();
  }
  g_free(chrRetval);
}

bool glom_python_module_is_available()
{
  PyObject* module_glom = PyImport_ImportModule((char*)"glom-" GLOM_ABI_VERSION); //TODO: unref this?
  return module_glom != 0;
}

bool gda_python_module_is_available()
{
  PyObject* module_glom = PyImport_ImportModule((char*)"gda"); //TODO: unref this?
  return module_glom != 0;
}



void glom_execute_python_function_implementation(const Glib::ustring& func_impl, const type_map_fields& field_values, Document* pDocument, const Glib::ustring& table_name, const Glib::RefPtr<Gnome::Gda::Connection>& opened_connection)
{
  glom_evaluate_python_function_implementation(Field::TYPE_TEXT, func_impl, field_values, pDocument, table_name, opened_connection);
}

Gnome::Gda::Value glom_evaluate_python_function_implementation(Field::glom_field_type result_type, const Glib::ustring& func_impl,
    const type_map_fields& field_values, Document* pDocument, const Glib::ustring& table_name, const Glib::RefPtr<Gnome::Gda::Connection>& opened_connection)
{
  //std::cout << "glom_evaluate_python_function_implementation()" << std::endl;
  //for(type_map_fields::const_iterator iter = field_values.begin(); iter != field_values.end(); ++iter)
  //{
  //  std::cout << "  field_value: name=" << iter->first << ", value=" << iter->second.to_string() << std::endl; 
  //}

  g_assert(result_type != Field::TYPE_INVALID);

  //g_warning("glom_evaluate_python_function_implementation: func=%s", func_impl.c_str());

  Gnome::Gda::Value valueResult;

  Glib::ustring func_def;

  //Indent the function implementation (required by python syntax):
  typedef std::list<Glib::ustring> type_listStrings;
  type_listStrings listStrings = ustring_tokenize(func_impl, "\n", -1);
  for(type_listStrings::const_iterator iter = listStrings.begin(); iter != listStrings.end(); ++iter)
  {
    func_def += "  " + *iter + "\n";
  }


  //prefix the def line:
  const Glib::ustring func_name = "glom_calc_field_value";
  func_def = "def " + func_name + "(record):\n  import glom-" GLOM_ABI_VERSION "\n  import gda\n" + func_def;
  //We did this in main(): Py_Initialize();

  PyObject* pMain = PyImport_AddModule((char*)"__main__");
  PyObject* pDict = PyModule_GetDict(pMain);


  //Allow the function to import from our script library:
  if(pDocument)
  {
    const std::vector<Glib::ustring> module_names = pDocument->get_library_module_names();
    for(std::vector<Glib::ustring>::const_iterator iter = module_names.begin(); iter != module_names.end(); ++iter)
    {
      const Glib::ustring name = *iter;
      const Glib::ustring script = pDocument->get_library_module(name);
      if(!name.empty() && !script.empty())
      {
        PyObject* objectCompiled = Py_CompileString(script.c_str(), name.c_str() /* "filename", for debugging info */,  Py_file_input /* "start token" for multiple lines of code. */); //Returns a reference.
  
        if(!objectCompiled)
          HandlePythonError();

        PyObject* pObject = PyImport_ExecCodeModule(const_cast<char*>(name.c_str()), objectCompiled); //Returns a reference. //This should make it importable.

        if(!pObject)
         HandlePythonError();

        Py_DECREF(pObject);
        Py_DECREF(objectCompiled);
        //TODO: When do these stop being importable? Should we unload them somehow later?
      }
    }
  }


  PyObject* module_glom = PyImport_ImportModule((char*)"glom-" GLOM_ABI_VERSION);
  if(!module_glom)
  {
    g_warning("Could not import python glom module.");
    return valueResult; // don't crash
  }

  PyObject* module_glom_dict = PyModule_GetDict(module_glom);
  //This seems to be different to PyGlomRecord_GetPyType() - we can PyObject_Call() this one to instantiate it.
  PyObject* pyTypeGlomRecord = PyDict_GetItemString(module_glom_dict, (char*)"Record"); //TODO: Unref this?
  if(!pyTypeGlomRecord || !PyType_Check(pyTypeGlomRecord))
  {
    g_warning("Could not get glom.Record from glom_module.");
    return valueResult; // don't crash
  }


  PyObject* module_gda = PyImport_ImportModule((char*)"gda");
  if(!module_gda)
  {
    g_warning("Could not import python gda module.");
    return valueResult;
  }

  // Gda.Value does not exist anymore in pygda-3.0
#if 0
  PyObject* module_gda_dict = PyModule_GetDict(module_gda);
  PyObject* pyTypeGdaValue = PyDict_GetItemString(module_gda_dict, "Value"); //TODO: Unref this?
  if(!pyTypeGdaValue || !PyType_Check(pyTypeGdaValue))
    g_warning("Could not get gda.Value from gda_module.");
#endif


  //Create the function definition:
  PyObject* pyValue = PyRun_String(func_def.c_str(), Py_file_input, pDict, pDict);
  if(pyValue)
  {
    Py_DECREF(pyValue);
    pyValue = 0;
  }
  else
  {
    ShowTrace();
  }

  //Call the function:
  {
    PyObject* pFunc = PyDict_GetItemString(pDict, func_name.c_str()); //The result is borrowed, so should not be dereferenced.
    if(!pFunc)
    {
      HandlePythonError();
    }

    if(!PyCallable_Check(pFunc))
    {
      HandlePythonError();
      g_warning("pFunc is not callable.");
      return valueResult;
    }

    //The function's parameter:
  
    //PyObject* pParam = PyString_FromString("test value"); //This test did not need the extra ref.

    PyObject* new_args = PyTuple_New(0);
    PyGlomRecord* pParam = (PyGlomRecord*)PyObject_Call((PyObject*)pyTypeGlomRecord, new_args, 0);
    //PyGlomRecord* pParam = (PyGlomRecord*)PyObject_Call((PyObject*)PyGlomRecord_GetPyType(), new_args, 0);
    Py_DECREF(new_args);
    if(pParam)
    {
      Py_INCREF(pParam); //TODO: As I understand it, PyObject_New() should return a new reference, so this should not be necessary.

      //Fill the record's details:
      PyGlomRecord_SetFields(pParam, field_values, pDocument, table_name, opened_connection);

      PyObject* pArgs = PyTuple_New(1);
      PyTuple_SetItem(pArgs, 0, (PyObject*)pParam); //The pParam reference is taken by PyTuple_SetItem().

      //Call the function with this parameter:
      PyObject* pyResult = PyObject_CallObject(pFunc, pArgs);
      Py_DECREF(pArgs);

      if(!pyResult)
      {
        g_warning("pyResult was null");
        HandlePythonError();
      }
      else
      {
        //Deal with the various possible return types:
        bool object_is_gda_value = false;

        GValue value = {0, {{0}}};
        const int test = pygda_value_from_pyobject(&value, pyResult);

        if(test == 0) //-1 means error.
          object_is_gda_value = true;

        if(object_is_gda_value && G_IS_VALUE(&value))
        {
          valueResult = Gnome::Gda::Value(&value);
          //Make sure that the value is of the expected Gda type:
          //TODO_Performance:
          valueResult = Glom::Conversions::convert_value(valueResult, result_type);
          //std::cout << "DEBUG: glom_evaluate_python_function_implementation(): valueResult Gda type=" << g_type_name(valueResult.get_value_type()) << std::endl;
          g_value_unset(&value);
        }
        else
        {
          //g_warning("debug: pyResult is not a gda.value");

          //For instance, if one of the fields was empty, then the calculation might want to return an empty value,
          //instead of returning 0.
          if(pyResult == Py_None) //Direct comparison is possible and recommended, because there is only one pyNone object.
          {
 
            //The result should be an appropriate empty value for this field type:
            valueResult = Conversions::get_empty_value(result_type);
            //std::cout << "DEBUG: glom_evaluate_python_function_implementation(): empty value Gda type=" << g_type_name(valueResult.get_value_type()) << std::endl;
          }
          else
          {
            //TODO: Handle numeric/date/time types:
            //(though I don't think this code is ever reached. murrayc)

            //Treat this as a string or something that can be converted to a string:
            PyObject* pyObjectResult = PyObject_Str(pyResult);
            if(PyString_Check(pyObjectResult))
            {
              const char* pchResult = PyString_AsString(pyObjectResult);
              if(pchResult)
              {
                bool success = false;
                valueResult = Conversions::parse_value(result_type, pchResult, success, true /* iso_format */);
                std::cout << "DEBUG: glom_evaluate_python_function_implementation(): parsed value Gda type=" << g_type_name(valueResult.get_value_type()) << std::endl;
 
              }
              else
                HandlePythonError();
            }
            else
              HandlePythonError();
          }
        }

        Py_DECREF(pyResult);
      }
    }
  }

  Py_FlushLine();
  PyErr_Clear();


  //We did this in main(): Py_Finalize();

  return valueResult;
}

/*

examples:

  return record.fields["name_first"] + " " + record.fields["name_last"]

  return record.related_records["contacts"][0].fields["name_first"]

  return record.related_records["invoice lines"].sum("cost") )
*/

} //namespace Glom
