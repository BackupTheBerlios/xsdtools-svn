/*  
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __SCHEMA_H__
#define __SCHEMA_H__

#include "complex.h"

namespace lang {
        bool isSimple (const std::string& type);
        std::string sqlType (const std::string& type);
        std::string javaName (std::string name);
        std::string javaType (const std::string& type);
}

struct Field {
	std::string name;
	std::string type;
	Field (const std::string& n = "", const std::string& t = "") : name (n), type (t) {}
};

struct Table {
	bool isLink;
	std::string name;
	std::string pk;
	std::vector<Field> field;
	Table (const std::string& n = "", const std::string& p = "") : name (n), pk (p), isLink (false) {}
};

struct FK {
	std::string table1;
	std::string field1;
        std::string table2;
        std::string field2;
	FK (const std::string& t1 = "", const std::string& f1 = "", const std::string& t2 = "", const std::string& f2 = "") : table1 (t1), field1 (f1), table2 (t2), field2 (f2) {}
};

class Schema {
	std::vector<Table> table;
	std::vector<FK> fk;

	void buildTable (const ComplexType& it, const std::vector<std::string>& individual, const std::vector<ComplexType>& sys, std::string& sd);
	void buildSQLModel (const std::vector<std::string>& individual);

	void resolve ();
	std::string getName (const std::string& ref);
public:
	std::vector<std::string> name;
	std::vector<ComplexType> type;
	std::vector<Element> element;
	std::string javaPackage;

	void load (const std::string& xsd, const std::vector<std::string>& individual = std::vector<std::string> ());
	std::string toHbm (const std::string& name);
	
	std::vector<std::string> listTables () const;
	std::vector<std::string> listClasses () const;
};

#endif

