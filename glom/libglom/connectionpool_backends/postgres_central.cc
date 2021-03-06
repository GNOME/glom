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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include <libglom/connectionpool_backends/postgres_central.h>

// Uncomment to see debug messages
//#define GLOM_CONNECTION_DEBUG

namespace Glom
{

namespace ConnectionPoolBackends
{

PostgresCentralHosted::PostgresCentralHosted()
: m_list_ports( {
    "5432", //Ubuntu Breezy seems to default to this for Postgres 7.4, and this is probably the default for most postgres installations, including Fedora.
    "5433", //Ubuntu Dapper seems to default to this for Postgres 8.1, probably to avoid a clash with Postgres 7.4
    "5434", //Earlier versions of Ubuntu Feisty defaulted to this for Postgres 8.2.
    "5435", //In case Ubuntu increases the port number again in future.
    "5436"} ), //In case Ubuntu increases the port number again in future.
  m_try_other_ports(true)
{
}

void PostgresCentralHosted::set_host(const Glib::ustring& value)
{
  if(value != m_host)
  {
    m_host = value;

    // Force us to try all ports again when connecting for the first time, then remember the working port again. Except when a specific port was set to be used.
    if(m_try_other_ports)
      m_port = 0;
  }
}

void PostgresCentralHosted::set_port(unsigned int port)
{
  m_port = port;
}

void PostgresCentralHosted::set_try_other_ports(bool val)
{
  m_try_other_ports = val;
}

Glib::ustring PostgresCentralHosted::get_host() const
{
  return m_host;
}

unsigned int PostgresCentralHosted::get_port() const
{
  return m_port;
}

bool PostgresCentralHosted::get_try_other_ports() const
{
  return m_try_other_ports;
}

Glib::RefPtr<Gnome::Gda::Connection> PostgresCentralHosted::connect(const Glib::ustring& database, const Glib::ustring& username, const Glib::ustring& password, bool fake_connection)
{
  Glib::RefPtr<Gnome::Gda::Connection> connection;

  //Try each possible network port:
  auto iter_port = m_list_ports.begin();

  //Start with the remembered-as-working port:
  auto port = port_as_string(m_port);
  if(m_port == 0)
    port = *iter_port ++;

  bool connection_possible = false;
  try
  {
    connection = attempt_connect(port, database, username, password, fake_connection);
    connection_possible = true;
    m_port = std::stoi(port);
  }
  catch(const ExceptionConnection& ex)
  {
    // Remember port if only the database was missing
    connection_possible = false;
    if(ex.get_failure_type() == ExceptionConnection::failure_type::NO_DATABASE)
    {
      connection_possible = true;
      m_port = std::stoi(port);
    }
  }

  // Try more ports if so desired, and we don't have a connection yet
  if(m_try_other_ports && !connection)
  {
    while(!connection && iter_port != m_list_ports.end())
    {
      port = *iter_port;

      try
      {
        connection = attempt_connect(port, database, username, password, fake_connection);
        connection_possible = true;
        m_port = std::stoi(port);
      }
      catch(const ExceptionConnection& ex)
      {
        //Don't set this, because we might have previously set it to true to
        //show that a connection was possible with a previously-tried port: connection_possible = false;

        // Remember port if only the database was missing
        if(ex.get_failure_type() == ExceptionConnection::failure_type::NO_DATABASE)
        {
          connection_possible = true;
          m_port = std::stoi(port);
        }
      }

      // Skip if we already tried this port
      if(iter_port != m_list_ports.end() && *iter_port == port)
        ++ iter_port;
    }
  }

  if(connection)
  {
    //Remember working port:
    m_port = std::stoi(port);
  }
  else
  {
    if(connection_possible)
      throw ExceptionConnection(ExceptionConnection::failure_type::NO_DATABASE);
    else
      throw ExceptionConnection(ExceptionConnection::failure_type::NO_SERVER);
  }

  return connection;
}

bool PostgresCentralHosted::create_database(const SlotProgress& slot_progress, const Glib::ustring& database_name, const Glib::ustring& username, const Glib::ustring& password)
{
  return attempt_create_database(slot_progress, database_name, get_host(), port_as_string(m_port), username, password);
}

}

}
