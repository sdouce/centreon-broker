/*
** Copyright 2015 Merethis
**
** This file is part of Centreon Broker.
**
** Centreon Broker is free software: you can redistribute it and/or
** modify it under the terms of the GNU General Public License version 2
** as published by the Free Software Foundation.
**
** Centreon Broker is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Centreon Broker. If not, see
** <http://www.gnu.org/licenses/>.
*/

#include <QMutexLocker>
#include <QFileInfo>
#include <QFile>
#include <QDirIterator>
#include <cstdio>
#include <set>
#include "com/centreon/broker/multiplexing/publisher.hh"
#include "com/centreon/broker/misc/tokenizer.hh"
#include "com/centreon/broker/dumper/directory_dumper.hh"
#include "com/centreon/broker/dumper/internal.hh"
#include "com/centreon/broker/dumper/dump.hh"
#include "com/centreon/broker/dumper/reload.hh"
#include "com/centreon/broker/dumper/remove.hh"
#include "com/centreon/broker/extcmd/command_request.hh"
#include "com/centreon/broker/extcmd/command_result.hh"
#include "com/centreon/broker/io/events.hh"
#include "com/centreon/broker/io/exceptions/shutdown.hh"
#include "com/centreon/broker/exceptions/msg.hh"
#include "com/centreon/broker/logging/logging.hh"

using namespace com::centreon::broker;
using namespace com::centreon::broker::dumper;

/**************************************
*                                     *
*           Public Methods            *
*                                     *
**************************************/

/**
 *  Constructor.
 *
 *  @param[in] name    The dumper endpoint name.
 *  @param[in] path    Dumper path.
 *  @param[in] tagname Dumper tagname.
 *  @param[in] cache   The persistent cache.
 */
directory_dumper::directory_dumper(
          std::string const& name,
          std::string const& path,
          std::string const& tagname,
          misc::shared_ptr<persistent_cache> cache)
  : _name(name.c_str()),
    _path(path),
    _tagname(tagname),
    _cache(cache) {
  try {
    _load_cache();
  }
  catch (std::exception const& e) {
    throw (exceptions::msg()
             << "directory_dumper: couldn't load cache for '"
             << _name << "': " << e.what());
  }
}


/**
 *  Destructor.
 */
directory_dumper::~directory_dumper() {
  try {
    _save_cache();
  }
  catch (std::exception const& e) {
    logging::error(logging::high)
      << "directory_dumper: couldn't save cache for '"
      << _name << "': " << e.what();
  }
}

/**
 *  Read data from the dumper.
 *
 *  @param[out] d         Next available event.
 *  @param[in]  deadline  Timeout.
 *
 *  @return Respect io::stream::read()'s return value.
 */
bool directory_dumper::read(
                         misc::shared_ptr<io::data>& d,
                         time_t deadline) {
  (void)d;
  (void)deadline;
  throw (io::exceptions::shutdown(true, false));
  return (true);
}

/**
 *  Write data to the dumper.
 *
 *  @param[in] d Data to write.
 *
 *  @return Always return 1, or throw exceptions.
 */
unsigned int directory_dumper::write(misc::shared_ptr<io::data> const& d) {
  if (d.isNull())
    return (1);

  if (d->type() == extcmd::command_request::static_type()) {
    extcmd::command_request const&
      req(d.ref_as<extcmd::command_request const>());
    if ((!req.destination_id
         || (req.destination_id == io::data::broker_id))
        && (req.endp == _name)) {
      try {
        // Execute command that was especially addressed to us.
        if (req.cmd == "DUMP_DIR")
          _dump_dir(_path);
        else
          throw (exceptions::msg() << "unknown command:"
                 << " the only valid command is DUMP_DIR");

        // Send successful result.
        misc::shared_ptr<extcmd::command_result>
          res(new extcmd::command_result);
        res->id = req.id;
        res->msg = "Command successfully executed.";
        res->code = 0;
        res->destination_id = req.source_id;
        multiplexing::publisher().write(res);
      }
      catch (std::exception const& e) {
        // Log error.
        logging::error(logging::medium)
          << "directory_dumper: couldn't parse '"
          << req.cmd << "': " << e.what();

        // Send error result.
        misc::shared_ptr<extcmd::command_result>
          res(new extcmd::command_result);
        res->id = req.id;
        res->msg = e.what();
        res->code = -1;
        res->destination_id = req.source_id;
        multiplexing::publisher().write(res);
      }
    }
  }

  return (1);
}

/**
 *  Dump the directory of this directory dumper.
 *
 *  @param[in] path  The path of the directory to dump.
 */
void directory_dumper::_dump_dir(std::string const& path) {
  logging::debug(logging::medium)
    << "directory_dumper: dumping dir '" << path << "'";

  multiplexing::publisher pblsh;

  QDirIterator dir(
    QString::fromStdString(path),
    QDir::Files | QDir::NoDotAndDotDot,
    QDirIterator::Subdirectories);

  QDir root_dir(QString::fromStdString(path));

  // Set of found files.
  std::set<std::string> found;

  while (dir.hasNext()) {
    QString path = dir.next();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
      logging::error(logging::medium)
        << "directory_dumper: can't read file '" << path << "'";
    QByteArray content = file.readAll();
    misc::shared_ptr<dump> dmp(new dump);
    dmp->filename = root_dir.relativeFilePath(path);
    dmp->content = QString(content);
    dmp->tag = QString::fromStdString(_tagname);
    pblsh.write(dmp);
    found.insert(dmp->filename.toStdString());
  }

  // Remove all files not found.
  for (std::map<std::string, timestamp_cache>::const_iterator
         it = _files_cache.begin(),
         end = _files_cache.end();
       it != end;
       ++it)
    if (found.find(it->first) == found.end()) {
      misc::shared_ptr<remove> rm(new remove);
      rm->tag = QString::fromStdString(_tagname);
      rm->filename = QString::fromStdString(it->first);
      pblsh.write(rm);
    }

  // Update the list of files found.
  _files_cache.clear();
  for (std::set<std::string>::const_iterator
         it = found.begin(),
         end = found.end();
       it != end;
       ++it) {
    timestamp_cache tc;
    tc.filename = QString::fromStdString(*it);
    _files_cache[*it] = tc;
  }

  // Ask for a reload.
  misc::shared_ptr<reload> rl(new reload);
  rl->tag = QString::fromStdString(_tagname);
  pblsh.write(rl);
}

/**
 *  Load the cache.
 */
void directory_dumper::_load_cache() {
  // No cache, nothing to do.
  if (_cache.isNull())
    return ;

  misc::shared_ptr<io::data> d;
  while (true) {
    _cache->get(d);
    if (d.isNull())
      return ;
    if (d->type() == timestamp_cache::static_type()) {
     timestamp_cache const& tc  = d.ref_as<timestamp_cache const>();
      _files_cache[tc.filename.toStdString()] = tc;
    }
  }
}

/**
 *  Save the cache.
 */
void directory_dumper::_save_cache() {
  // No cache, nothing to do.
  if (_cache.isNull())
    return ;

  _cache->transaction();
  for (std::map<std::string, timestamp_cache>::const_iterator
         it = _files_cache.begin(),
         end = _files_cache.end();
       it != end;
       ++it)
    _cache->add(misc::make_shared(new timestamp_cache(it->second)));
  _cache->commit();
}
