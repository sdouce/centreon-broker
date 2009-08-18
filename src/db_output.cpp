/*
**  Copyright 2009 MERETHIS
**  This file is part of CentreonBroker.
**
**  CentreonBroker is free software: you can redistribute it and/or modify it
**  under the terms of the GNU General Public License as published by the Free
**  Software Foundation, either version 2 of the License, or (at your option)
**  any later version.
**
**  CentreonBroker is distributed in the hope that it will be useful, but
**  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
**  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
**  for more details.
**
**  You should have received a copy of the GNU General Public License along
**  with CentreonBroker.  If not, see <http://www.gnu.org/licenses/>.
**
**  For more information : contact@centreon.com
*/

#include <boost/bind.hpp>
#include <memory>
#include "db/db_exception.h"
#ifdef USE_MYSQL
# include "db/mysql/connection.h"
#endif /* USE_MYSQL */
#ifdef USE_POSTGRESQL
# include "db/postgresql/connection.h"
#endif /* USE_POSTGRESQL */
#include "db/predicate.h"
#include "db/truncate.h"
#include "db_output.h"
#include "exception.h"
#include "events/comment.h"
#include "events/event.h"
#include "events/host.h"
#include "events/host_status.h"
#include "logging.h"

using namespace CentreonBroker;
using namespace CentreonBroker::Events;

/**************************************
*                                     *
*           Private Methods           *
*                                     *
**************************************/

/**
 *  \brief DBOutput copy constructor.
 *
 *  As DBOutput are not copiable, the copy constructor is declared private.
 *
 *  \param[in] dbo Unused.
 */
DBOutput::DBOutput(const DBOutput& dbo) : EventSubscriber()
{
  (void)dbo;
  assert(false);
}

/**
 *  \brief Overload of the assignment operator.
 *
 *  As DBOutput are not copiable, the assignment operator is declared private.
 *
 *  \param[in] dbo Unused.
 *
 *  \return *this
 */
DBOutput& DBOutput::operator=(const DBOutput& dbo)
{
  (void)dbo;
  assert(false);
  return (*this);
}

/**
 *  Purge a table before starting event processing.
 */
void DBOutput::CleanTable(const std::string& table)
{
#ifndef NDEBUG
  {
    std::string debug;

    debug = "Truncating table `";
    debug += table;
    debug += "`...";
    logging.LogDebug(debug.c_str());
    logging.Indent();
  }
#endif /* !NDEBUG */
  std::auto_ptr<DB::Truncate> truncate(this->conn_->GetTruncate());

  truncate->SetTable(table);
  truncate->Execute();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Purge tables before starting event processing.
 */
void DBOutput::CleanTables()
{
#ifndef NDEBUG
  logging.LogDebug("Initial table purge...", true);
#endif /* !NDEBUG */
  this->CleanTable(this->acknowledgement_mapping_.GetTable());
  this->CleanTable(this->comment_mapping_.GetTable());
  this->CleanTable(this->connection_mapping_.GetTable());
  this->CleanTable(this->downtime_mapping_.GetTable());
  this->CleanTable(this->host_mapping_.GetTable());
  this->CleanTable(this->host_group_mapping_.GetTable());
  this->CleanTable(this->program_status_mapping_.GetTable());
  this->CleanTable(this->service_mapping_.GetTable());
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  \brief Commit the current transaction and reset timeout.
 *
 *  Ask the underlying database to commit the current transaction. The timeout
 *  for autocommit is also rescheduled.
 */
void DBOutput::Commit()
{
  this->conn_->Commit();
  this->queries_ = 0;
  // XXX : timeout should be configurable
  this->timeout_ = boost::get_system_time() + boost::posix_time::seconds(7);
  return ;
}

/**
 *  \brief Connect to the preconfigured database.
 *
 *  Using parameters provided by Init(), connect to the database server. Upon
 *  successful return, the connection can later be dropped by calling
 *  Disconnect().
 *
 *  \see Disconnect
 *  \see Init
 */
void DBOutput::Connect()
{
  // Connect to the DB server
  switch (this->dbms_)
    {
#ifdef USE_MYSQL
     case DB::Connection::MYSQL:
      this->conn_ = new DB::MySQLConnection;
      break;
#endif /* USE_MYSQL */

#ifdef USE_POSTGRESQL
     case DB::Connection::POSTGRESQL:
      this->conn_ = new DB::PgSQLConnection;
      break ;
#endif /* USE_POSTGRESQL */

     default:
      throw (Exception(0, "Unsupported DBMS requested."));
    }
  this->conn_->Connect(this->host_,
                       this->user_,
                       this->password_,
                       this->db_);

  // Deactivate auto-commit.
  this->conn_->AutoCommit(false);

  // Initialize internal auto-commit timeout
  this->Commit();

  // XXX : synchronize data with server
  /*
    std::auto_ptr<DB::Select> query = this->conn_->GetSelectQuery();
    query->AddField("instance_id");
    query->AddField("instance_name");
    query->SetTable("program_status");
    query->Prepare();
    query->Execute();
    while (query.HasNext())
    {
      this->instances_[query->GetString("instance_name")] = query->GetInt("instance_id");
    }
  */

  return ;
}

/**
 *  \brief Disconnect from the database server.
 *
 *  Upon a successful call to Connect(), one can disconnect from the server by
 *  calling this method. All previously allocated ressources are freed.
 *
 *  \see Connect
 */
void DBOutput::Disconnect()
{
  // XXX : delete statements
  if (this->conn_)
    {
      delete (this->conn_);
      this->conn_ = NULL;
    }
  return ;
}

/**
 *  \brief Get the ID of an instance by its name.
 *
 *  The DBOutput class caches instance IDs as those are used within almost
 *  every table of the schema. This avoids expensive SELECT queries.
 *
 *  \param[in] instance The name of the Nagios instance.
 *
 *  \return The database ID of the Nagios instance.
 */
int DBOutput::GetInstanceId(const std::string& instance)
{
  int id;
  // XXX : instance ids should be synchronized with DB
  std::map<std::string, int>::iterator it;

  it = this->instances_.find(instance);
  if (it == this->instances_.end())
    {
      id = this->instances_.size() + 1;
      this->instances_[instance] = id;
    }
  else
    id = this->instances_[instance];
  return (id);
}

/**
 *  \brief Insert the given object into an internal list for later processing.
 *
 *  Reminder : we use a publish / subscribe model for event propagation.
 *
 *  When a new event is published, the EventPublisher instance will call this
 *  method (we subclass EventSubscriber). We will store the event into our
 *  internal list. This list is processed by a separate thread.
 *
 *  \param[in] e The newly generated event.
 */
void DBOutput::OnEvent(Event* e) throw ()
{
  try
    {
      this->events_.Add(e);
    }
  catch (...)
    {
      logging.LogError("Exception while adding event to list. Dropping event.");
      e->RemoveReader(this);
    }
  return ;
}

/**
 *  Initialize object-relational mappings.
 */
void DBOutput::PrepareMappings()
{
  // Connection mapping.
  this->connection_mapping_.AddIntField("instance_id",
					boost::bind(&DBOutput::GetInstanceId,
						    this,
						    boost::bind(
					  &Connection::instance,
					  _1)));

  // Host mapping.
  this->host_mapping_.AddIntField("instance_id",
				  boost::bind(&DBOutput::GetInstanceId,
					      this,
					      boost::bind(
                                                &Host::instance,
						_1)));

  // ProgramStatus mapping.
  this->program_status_mapping_.AddIntField("instance_id",
					    boost::bind(
					      &DBOutput::GetInstanceId,
					      this,
					      boost::bind(
                                          &ProgramStatus::instance,
					  _1)));

  // Service mapping.
  this->service_mapping_.AddIntField("instance_id",
				     boost::bind(&DBOutput::GetInstanceId,
						 this,
						 boost::bind(
				       &ProgramStatus::instance,
				       _1)));

  return ;
}

/**
 *  Prepare most often used statements.
 */
void DBOutput::PrepareStatements()
{
  // ConnectionStatus update statement.
  this->connection_status_stmt_ =
    this->conn_->GetMappedUpdate<ConnectionStatus>(
      this->connection_status_mapping_);
  this->connection_status_stmt_->SetTable("connection_info");
  this->connection_status_stmt_->SetPredicate(DB::Equal(
    DB::Field("instance_id"),
    DB::Placeholder()));
  this->connection_status_stmt_->Prepare();

  // HostStatus update statement.
  this->host_status_stmt_ = this->conn_->GetMappedUpdate<HostStatus>(
                              this->host_status_mapping_);
  this->host_status_stmt_->SetTable("host");
  this->host_status_stmt_->SetPredicate(
    DB::And(DB::Equal(DB::Field("instance_id"),
		      DB::Placeholder()),
	    DB::Equal(DB::Field("host_name"),
		      DB::Placeholder())
	    ));
  this->host_status_stmt_->Prepare();

  // ProgramStatus update statement.
  this->program_status_stmt_ = this->conn_->GetMappedUpdate<ProgramStatus>(
                                 this->program_status_mapping_);
  this->program_status_stmt_->SetTable("program_status");
  this->program_status_stmt_->SetPredicate(
    DB::Equal(DB::Field("instance_id"),
	      DB::Placeholder()));
  this->program_status_stmt_->Prepare();

  // ServiceStatus update statement.
  this->service_status_stmt_ = this->conn_->GetMappedUpdate<ServiceStatus>(
                                 this->service_status_mapping_);
  this->service_status_stmt_->SetTable("service");
  this->service_status_stmt_->SetPredicate(
    DB::And(DB::Equal(DB::Field("instance_id"),
		      DB::Placeholder()),
	    DB::And(DB::Equal(DB::Field("host_name"),
			      DB::Placeholder()),
		    DB::Equal(DB::Field("service_description"),
			      DB::Placeholder()))
	    ));
  this->service_status_stmt_->Prepare();
  return ;
}

/**
 *  \brief Process an event.
 *
 *  When an event is poped from the list, it will be processed by this method.
 *  We will determine the true event type and process it accordingly.
 *
 *  \param[in] event Event that should be stored in the database.
 */
void DBOutput::ProcessEvent(Event* event)
{
  switch (event->GetType())
    {
     case Event::ACKNOWLEDGEMENT:
      ProcessAcknowledgement(*static_cast<Acknowledgement*>(event));
      break ;
     case Event::COMMENT:
      ProcessComment(*static_cast<Comment*>(event));
      break ;
     case Event::CONNECTION:
      ProcessConnection(*static_cast<Connection*>(event));
      break ;
     case Event::CONNECTIONSTATUS:
      ProcessConnectionStatus(*static_cast<ConnectionStatus*>(event));
      break ;
     case Event::DOWNTIME:
      ProcessDowntime(*static_cast<Downtime*>(event));
      break ;
     case Event::HOST:
      ProcessHost(*static_cast<Host*>(event));
      break ;
     case Event::HOSTGROUP:
      ProcessHostGroup(*static_cast<HostGroup*>(event));
      break ;
     case Event::HOSTSTATUS:
      ProcessHostStatus(*static_cast<HostStatus*>(event));
      break ;
     case Event::LOG:
      ProcessLog(*static_cast<Log*>(event));
      break ;
     case Event::PROGRAMSTATUS:
      ProcessProgramStatus(*static_cast<ProgramStatus*>(event));
      break ;
     case Event::SERVICE:
      ProcessService(*static_cast<Service*>(event));
      break ;
     case Event::SERVICESTATUS:
      ProcessServiceStatus(*static_cast<ServiceStatus*>(event));
      break ;
     default:
      assert(false);
      throw (Exception(event->GetType(), "Invalid event type encountered"));
    }
  event->RemoveReader(this);
  return ;
}

/**
 *  Process an Acknowledgement event.
 */
void DBOutput::ProcessAcknowledgement(const Acknowledgement& ack)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Acknowledgement event...");
  logging.Indent();
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Acknowledgement> >
    query(this->conn_->GetMappedInsert<Acknowledgement>(
            this->acknowledgement_mapping_));

  query->SetTable("acknowledgement");
  query->SetArg(ack);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a Comment event.
 */
void DBOutput::ProcessComment(const Comment& comment)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Comment event...");
  logging.Indent();
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Comment> >
    query(this->conn_->GetMappedInsert<Comment>(this->comment_mapping_));

  query->SetTable("comment");
  query->SetArg(comment);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a Connection event.
 */
void DBOutput::ProcessConnection(const Connection& connection)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Connection event...");
  logging.Indent();
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Connection> >
    query(this->conn_->GetMappedInsert<Connection>(
            this->connection_mapping_));

  query->SetTable("connection_info");
  query->SetArg(connection);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}


/**
 *  Process a ConnectionStatus event.
 */
void DBOutput::ProcessConnectionStatus(const ConnectionStatus& cs)
{
#ifndef NDEBUG
  logging.LogDebug("Processing ConnectionStatus event...");
  logging.Indent();
#endif /* !NDEBUG */
  try
    {
      this->connection_status_stmt_->SetArg(cs);
      this->connection_status_stmt_->Execute();
    }
  catch (DB::DBException& dbe)
    {
      if (dbe.GetReason() != DB::DBException::QUERY_EXECUTION)
	{
#ifndef NDEBUG
	  logging.Deindent();
#endif /* !NDEBUG */
	  throw ;
	}
    }
  if (this->connection_status_stmt_->GetUpdateCount() == 0)
    this->ProcessConnection(Connection(cs));
  else
    this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a Downtime event.
 */
void DBOutput::ProcessDowntime(const Downtime& downtime)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Downtime event...", true);
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Downtime> >
    query(this->conn_->GetMappedInsert<Downtime>(this->downtime_mapping_));

  query->SetTable("downtime");
  query->SetArg(downtime);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process an Host event.
 */
void DBOutput::ProcessHost(const Host& host)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Host event...", true);
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Host> >
    query(this->conn_->GetMappedInsert<Host>(this->host_mapping_));

  query->SetTable("host");
  query->SetArg(host);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a HostGroup event.
 */
void DBOutput::ProcessHostGroup(const HostGroup& hg)
{
#ifndef NDEBUG
  logging.LogDebug("Processing HostGroup event...", true);
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<HostGroup> >
    query(this->conn_->GetMappedInsert<HostGroup>(this->host_group_mapping_));

  query->SetTable("host_group");
  query->SetArg(hg);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a HostStatus event.
 */
void DBOutput::ProcessHostStatus(const HostStatus& hs)
{
#ifndef NDEBUG
  logging.LogDebug("Processing HostStatus event...", true);
#endif /* !NDEBUG */
  try
    {
      this->host_status_stmt_->SetArg(hs);
      this->host_status_stmt_->Execute();
    }
  catch (DB::DBException& dbe)
    {
      if (dbe.GetReason() != DB::DBException::QUERY_EXECUTION)
	{
#ifndef NDEBUG
	  logging.Deindent();
#endif /* !NDEBUG */
	  throw ;
	}
    }
  if (this->host_status_stmt_->GetUpdateCount() == 0)
    this->ProcessHost(Host(hs));
  else
    this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a Log event.
 */
void DBOutput::ProcessLog(const Log& log)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Log event...", true);
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Log> >
    query(this->conn_->GetMappedInsert<Log>(log_mapping));

  query->SetTable("log");
  query->SetArg(log);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a ProgramStatus event.
 */
void DBOutput::ProcessProgramStatus(const ProgramStatus& ps)
{
#ifndef NDEBUG
  logging.LogDebug("Processing ProgramStatus event...");
  logging.Indent();
#endif /* !NDEBUG */
  try
    {
      this->program_status_stmt_->SetArg(ps);
      ((DB::HaveArgs*)(this->program_status_stmt_))->SetArg(this->GetInstanceId(ps.instance));
      this->program_status_stmt_->Execute();
    }
  catch (DB::DBException& dbe)
    {
      if (dbe.GetReason() != DB::DBException::QUERY_EXECUTION)
	{
#ifndef NDEBUG
	  logging.Deindent();
#endif /* !NDEBUG */
	  throw ;
	}
    }
  if (this->program_status_stmt_->GetUpdateCount() == 0)
    {
      std::auto_ptr<DB::MappedInsert<ProgramStatus> >
        query(this->conn_->GetMappedInsert<ProgramStatus>(
          this->program_status_mapping_));

      query->SetTable("program_status");
      query->SetArg(ps);
      query->Execute();
      this->QueryExecuted();
    }
  else
    this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a Service event.
 */
void DBOutput::ProcessService(const Service& service)
{
#ifndef NDEBUG
  logging.LogDebug("Processing Service event...");
  logging.Indent();
#endif /* !NDEBUG */
  std::auto_ptr<DB::MappedInsert<Service> >
    query(this->conn_->GetMappedInsert<Service>(this->service_mapping_));

  query->SetTable("service");
  query->SetArg(service);
  query->Execute();
  this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Process a ServiceStatus event.
 */
void DBOutput::ProcessServiceStatus(const ServiceStatus& ss)
{
#ifndef NDEBUG
  logging.LogDebug("Processing ServiceStatus event...");
  logging.Indent();
#endif /* !NDEBUG */
  try
    {
      this->service_status_stmt_->SetArg(ss);
      this->service_status_stmt_->Execute();
    }
  catch (DB::DBException& dbe)
    {
      if (dbe.GetReason() != DB::DBException::QUERY_EXECUTION)
	throw ;
    }
  if (this->service_status_stmt_->GetUpdateCount() == 0)
    this->ProcessService(Service(ss));
  else
    this->QueryExecuted();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  Call this method when a query is executed.
 */
void DBOutput::QueryExecuted()
{
  // XXX : query number should be configurable
  if (++this->queries_ >= 10000)
    this->Commit();
  return ;
}

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  \brief DBOutput default constructor.
 *
 *  Initialize the DBOutput with default parameters.
 *
 *  \param[in] dbms Type of the database to use.
 */
DBOutput::DBOutput(DB::Connection::DBMS dbms)
  : acknowledgement_mapping_(acknowledgement_mapping),
    comment_mapping_(comment_mapping),
    connection_mapping_(connection_mapping),
    connection_status_mapping_(connection_status_mapping),
    downtime_mapping_(downtime_mapping),
    host_mapping_(host_mapping),
    host_group_mapping_(host_group_mapping),
    host_status_mapping_(host_status_mapping),
    program_status_mapping_(program_status_mapping),
    service_mapping_(service_mapping),
    service_status_mapping_(service_status_mapping),
    dbms_(dbms)
{
  this->PrepareMappings();
}

/**
 *  \brief DBOutput destructor.
 *
 *  Release all previously allocated ressources.
 */
DBOutput::~DBOutput()
{
#ifndef NDEBUG
  logging.LogDebug("Deleting DBOutput...");
#endif /* !NDEBUG */
  // XXX : statement deletion should be in Disconnect
  if (this->connection_status_stmt_)
    delete (this->connection_status_stmt_);
  if (this->host_status_stmt_)
    delete (this->host_status_stmt_);
  if (this->program_status_stmt_)
    delete (this->program_status_stmt_);
  if (this->service_status_stmt_)
    delete (this->service_status_stmt_);
  if (this->thread_)
    {
#ifndef NDEBUG
      logging.Indent();
      logging.LogDebug("Waiting for the running thread to finish");
#endif /* !NDEBUG */
      this->thread_->join();
      delete (this->thread_);
#ifndef NDEBUG
      logging.Deindent();
#endif /* !NDEBUG */
    }
}

/**
 *  \brief Entry point of the processing thread.
 *
 *  When Init() is called, it creates a new thread. This method is the thread
 *  entry point.
 */
void DBOutput::operator()()
{
  // Allocate printing ressources for this thread.
  logging.ThreadStart();

#ifndef NDEBUG
  logging.LogDebug("New thread created (DBOutput)");
#endif /* !NDEBUG */
  try
    {
#ifndef NDEBUG
      logging.LogDebug("Initial connection to the database...");
#endif /* !NDEBUG */
      this->Connect();
      this->CleanTables();
    }
  // An error occured on initialization, we won't try to go any further as it
  // is likely that something fatal happened (unreachable server, no memory
  // available, ...)
  catch (...)
    {
      logging.LogError("Initial database initialization failed...");
      logging.LogInfo("Aborting output...");
      this->Disconnect();
      return ;
    }

  while (!this->exit_)
    {
      Event* event;

      try
        {
	  this->PrepareStatements();
          while (1)
            {
              event = this->events_.TimedWait(this->timeout_);
              if (event)
                this->ProcessEvent(event);
              else if (!this->events_.Empty() || !this->exit_)
                this->Commit();
              else
		{
#ifndef NDEBUG
		  logging.LogDebug("DBOutput deletion requested, " \
                                   "exiting thread...");
#endif /* !NDEBUG */
		  break ;
		}
            }
        }

      // If such an exception occur, it's likely because the connection with
      // the DB server has been dropped. So we will disconnect and try to
      // reconnect later.
      catch (DB::DBException& dbe)
	{
	  logging.LogError("Recoverable DB error", true);
	  logging.LogError(dbe.what());
	  logging.Deindent();
	}

      // Standard exception should be thrown if an unrecoverable error occured.
      catch (std::exception& e)
        {
	  logging.LogError("Unrecoverable error", true);
	  logging.LogError(e.what());
	  logging.Deindent();
          break ;
        }
      catch (...)
	{
	  logging.LogError("Unknown unrecoverable error");
	  break ;
	}

      while (1)
	{
	  // Free connection ressources
	  this->Disconnect();

	  try
	    {
	      // XXX : retry interval should be configurable
	      // Wait before trying to reconnect.
	      boost::thread::sleep(boost::get_system_time()
				     + boost::posix_time::seconds(10));

	      if (!this->exit_)
		{
		  logging.LogInfo("Trying connection to DB server again...");
		  this->Connect();
		}
	      break ;
	    }
	  // If an exception occur, try to sleep a bit then try again
	  catch (...)
	    {
	    }
	}
      if (!this->exit_)
	break ;
    }
#ifndef NDEBUG
  logging.LogDebug("Exiting DBOutput thread");
#endif /* !NDEBUG */

  // Close connection
  this->Disconnect();

  // Release printing ressources for this thread.
  logging.ThreadEnd();
  return ;
}

/**
 *  \brief Request the object to stop processing.
 *
 *  This method requires the processing thread to stop.
 */
void DBOutput::Destroy()
{
#ifndef NDEBUG
  logging.LogDebug("Requesting DBOutput to stop processing...", true);
#endif /* !NDEBUG */
  assert(this->thread_);
  this->exit_ = true;
  this->events_.CancelWait();
  this->thread_->interrupt();
#ifndef NDEBUG
  logging.Deindent();
#endif /* !NDEBUG */
  return ;
}

/**
 *  \brief Initialize the object.
 *
 *  Set connection parameters and launch the processing thread.
 *
 *  \param[in] host     IP address or host name to connect to.
 *  \param[in] user     User name to use for authentication.
 *  \param[in] password Password to use for authentication.
 *  \param[in] db       Name of the database to connect to.
 */
void DBOutput::Init(const std::string& host,
                    const std::string& user,
                    const std::string& password,
                    const std::string& db)
{
  this->host_ = host;
  this->user_ = user;
  this->password_ = password;
  this->db_ = db;
  this->exit_ = false;
  this->thread_ = new boost::thread(boost::ref(*this));
  return ;
}
