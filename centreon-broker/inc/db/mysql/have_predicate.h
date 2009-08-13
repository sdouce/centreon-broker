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

#ifndef DB_MYSQL_HAVE_PREDICATE_H_
# define DB_MYSQL_HAVE_PREDICATE_H_

# include "db/have_predicate.h"
# include "db/predicate.h"

namespace                 CentreonBroker
{
  namespace               DB
  {
    class                 MySQLHavePredicate : virtual public HavePredicate,
                                               private PredicateVisitor
    {
     private:
      std::string*        query_;
      void                Visit(And& a_n_d);
      void                Visit(Equal& equal);
      void                Visit(Field& field);
      void                Visit(Placeholder& placeholder);
      void                Visit(Terminal& terminal);

     protected:
      unsigned int        placeholders;
                          MySQLHavePredicate();
                          MySQLHavePredicate(const MySQLHavePredicate& myhp);
      MySQLHavePredicate& operator=(const MySQLHavePredicate& myhp);

     public:
      virtual             ~MySQLHavePredicate();
      void                PreparePredicate(std::string& query);
      void                ProcessPredicate(std::string& query);
    };
  }
}

#endif /* !DB_MYSQL_HAVE_PREDICATE_H_ */
