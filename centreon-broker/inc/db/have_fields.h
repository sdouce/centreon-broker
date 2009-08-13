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

#ifndef DB_HAVE_FIELDS_H_
# define DB_HAVE_FIELDS_H_

# include <list>
# include <string>
# include "db/have_args.h"

namespace                    CentreonBroker
{
  namespace                  DB
  {
    class                    HaveFields : virtual public HaveArgs
    {
     protected:
      std::list<std::string> fields;
                             HaveFields();
                             HaveFields(const HaveFields& hf);
      HaveFields&            operator=(const HaveFields& hf);
      virtual                ~HaveFields();

     public:
      void                   AddField(const std::string& field);
      void                   RemoveField(const std::string& field);
    };
  }
}

#endif /* !DB_HAVE_FIELDS_H_ */
