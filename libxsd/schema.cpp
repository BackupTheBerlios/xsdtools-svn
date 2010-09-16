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

#include "schema.h"
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string.h>
#include <iostream>
#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>

namespace lang {
	bool isSimple (const std::string& type) {
	        if (!type.compare ("string") || !type.compare ("int") || !type.compare ("boolean") || !type.compare ("decimal") || !type.compare ("unsignedByte") || !type.compare ("dateTime")) return true;
	        return false;
	}

	std::string sqlType (const std::string& type) {
	        if (!type.compare ("string")) return "text";
		if (!type.compare ("unsignedByte")) return "int";
		if (!type.compare ("dateTime")) return "timestamp";
	        return type;
	}

	std::string javaName (std::string name) {
	        boost::replace_all (name, "_", "");
	        return name;
	}

	std::string javaType (std::string type) {
	        if (!type.compare ("serial")) return "int";
	        if (!type.compare ("string")) return "String";
		if (!type.compare ("decimal")) return "java.math.BigDecimal";
		if (!type.compare ("unsignedByte")) return "int";
		if (!type.compare ("dateTime")) return "java.sql.Timestamp";
	        return type;
	}
}

using namespace lang;

void Schema::load (const std::string& xsd, const std::vector<std::string>& individual) {
	xmlDocPtr doc = xmlReadFile (xsd.c_str (), NULL, 0);
	if (!doc) {
		throw "xsd parse error!";
	}

	xmlNode *el = xmlDocGetRootElement(doc);
	if (el && strncmp (reinterpret_cast<const char*>(el->name), "schema", 6) == 0) {
		std::string tmp = reinterpret_cast<const char*> (xmlGetProp (el, reinterpret_cast<const xmlChar*>("targetNamespace")));
		tmp = tmp.substr (7);
		size_t pos = tmp.find_first_of ('/');
		std::string tmp2 = tmp.substr (0, pos);
		tmp = tmp.substr (pos + 1);
		while ((pos = tmp2.find_last_of ('.')) != std::string::npos) {
			name.push_back (tmp2.substr (pos + 1));
			tmp2 = tmp2.substr (0, pos);
		}
		if (tmp2.length ()) name.push_back (tmp2);
		while ((pos = tmp.find_first_of ('/')) != std::string::npos) {
                        name.push_back (tmp.substr (0, pos));
                        tmp = tmp.substr (pos + 1);
                }
		if (tmp.length ()) name.push_back (tmp);

		el = el->children;
		for (; el; el = el->next) {
			if (strncmp (reinterpret_cast<const char*>(el->name), "complexType", 11) == 0) {
				xmlNode* seq = el->children;
				ComplexType cmplx;
				cmplx.name = (const char*) xmlGetProp (el, (const xmlChar*) "name");
				for (;seq; seq = seq->next) {
					if (strncmp (reinterpret_cast<const char*>(seq->name), "sequence", 8) == 0) {
						for (xmlNode* chld = seq->children; chld; chld = chld->next) {
							if (strncmp (reinterpret_cast<const char*>(chld->name), "element", 7) == 0) {
								Element e;
								if (xmlHasProp (chld, (const xmlChar*)"name")) e.name = (const char*)xmlGetProp (chld, (const xmlChar*)"name");
								if (xmlHasProp (chld, (const xmlChar*)"type")) e.type = (const char*)xmlGetProp (chld, (const xmlChar*)"type");
								if (xmlHasProp (chld, (const xmlChar*)"ref")) e.ref = (const char*)xmlGetProp (chld, (const xmlChar*)"ref");
								if (xmlHasProp (chld, (const xmlChar*)"maxOccurs")) e.maxOccurs = (const char*)xmlGetProp (chld, (const xmlChar*)"maxOccurs");
								if (xmlHasProp (chld, (const xmlChar*)"minOccurs")) e.minOccurs = (const char*)xmlGetProp (chld, (const xmlChar*)"minOccurs");
								cmplx.sequence.push_back (e);
							}
						}
					}
				}
				this->type.push_back (cmplx);
			} else if (strncmp (reinterpret_cast<const char*>(el->name), "element", 7) == 0) {
				Element e;
				if (xmlHasProp (el, (const xmlChar*)"name")) e.name = (const char*)xmlGetProp (el, (const xmlChar*)"name");
				if (xmlHasProp (el, (const xmlChar*)"type")) e.type = (const char*)xmlGetProp (el, (const xmlChar*)"type");
				this->element.push_back (e);
			}
		}
	} else {
		xmlFreeDoc(doc);
		throw "format error";	
	}

	xmlFreeDoc(doc);

	resolve ();

	buildSQLModel (individual);

        javaPackage = "";
        for (std::vector<std::string>::iterator it = this->name.begin (); it != this->name.end (); it++) {
                javaPackage += *it + ".";
        }
        javaPackage = javaPackage.substr (0, javaPackage.length () - 1);

}

void Schema::resolve () {
	for (std::vector<ComplexType>::iterator ct = type.begin (); ct != type.end (); ct++) {
		for (std::vector<Element>::iterator et = ct->sequence.begin (); et != ct->sequence.end (); et++) {
			if (et->ref.length ()) {
				et->type = getName (et->ref);
			}
			if (!et->type.substr (0,4).compare ("tns:")) {
				et->type = et->type.substr (4);
			}
		} 
	}
}

std::string Schema::getName (const std::string& ref) {
	if (!ref.substr (0, 4).compare ("tns:")) {
		for (std::vector<Element>::iterator et = element.begin (); et != element.end (); et++) {
			if (!et->name.compare (ref.substr (4))) return et->type.substr (4);
		}
	}
	return ref;
}

void Schema::buildTable (const ComplexType& it, const std::vector<std::string>& individual, const std::vector<ComplexType>& sys, std::string& sd) {
	size_t size = individual.size ();
		Table tmp (it.name, "id");
//		tmp.field.push_back (Field ("id", "serial"));
                for (std::vector<Element>::const_iterator it2 = it.sequence.begin (); it2 != it.sequence.end (); it2++) {
                        if (isSimple (it2->type)) {
                                tmp.field.push_back (Field (it2->name, it2->type));
                        } else {
				std::string type = it2->type;
				std::string name = it2->name;
                                for (size_t i = 0; i != size; i++) {
                                        if (!it2->type.compare (individual[i])) {
                                                sd += "0";
                                                ComplexType tmp2 = sys[i];
                                                type = tmp2.name = /*it2->type + */it2->name + "T";
						buildTable (tmp2, individual, sys, sd);
						break;
                                        }
                                }
                                if (/*(!it2->minOccurs.compare ("1") && !it2->maxOccurs.compare ("1")) || (!it2->minOccurs.length ())*/1) {
                                        if (!name.length ()) name = "id" + type;
                                        tmp.field.push_back (Field (name, "int"));
                                        fk.push_back (FK (it.name, name, type, "id"));
                                } else {
                                        Table tmp2 (it.name + "_" + type, "id");
//					tmp2.field.push_back (Field ("id", "serial"));
                                        tmp2.field.push_back (Field ("id" + it.name, "int"));
                                        tmp2.field.push_back (Field ("id" + type, "int"));
					tmp2.isLink = true;
                                        table.push_back (tmp2);
                                        fk.push_back (FK (it.name + "_" + type, "id" + it.name, it.name, "id"));
                                        fk.push_back (FK (it.name + "_" + type, "id" + type, type, "id"));
                                }
                        }
                }
                table.push_back (tmp);
}

void Schema::buildSQLModel (const std::vector<std::string>& individual) {
        std::string res;

        size_t size = individual.size ();
        std::vector<ComplexType> sys; sys.resize (size);
        for (std::vector<ComplexType>::iterator it = type.begin (); it != type.end (); it++) {
                for (size_t i = 0; i != size; i++) {
                        if (!it->name.compare (individual[i])) sys[i] = *it;
                }
        }
        std::string sd = "0";

        for (std::vector<ComplexType>::iterator it = type.begin (); it != type.end (); it++) {
		buildTable (*it, individual, sys, sd);
        }
}

std::string Schema::toSQL () {
	std::string res;
	for (size_t i = 0; i < table.size (); i++) {
		res += "CREATE TABLE \"" + table[i].name + "\"(\nid serial";
		for (size_t j = 0, size2 = table[i].field.size (); j < size2; j++) {
			res += "\n, \"" + table[i].field[j].name + "\" " + sqlType (table[i].field[j].type);
			if (!table[i].field[j].name.compare (table[i].pk)) res += " primary key";
//			if (j != size2 - 1) res += ",\n";
		}
		res += "\n, CONSTRAINT " + table[i].name + "_pkey PRIMARY KEY (id));\n";
	}

	for (size_t i = 0; i != this->fk.size (); i++) {
		res += "ALTER TABLE \"" + this->fk[i].table1 + "\" ADD CONSTRAINT f" + this->fk[i].table1 + this->fk[i].table2 + this->fk[i].field1 + " FOREIGN KEY (\"" + this->fk[i].field1 + "\") REFERENCES \"" + this->fk[i].table2 + "\"(" + this->fk[i].field2 + ");\n"; 
	}

	return res;
}

#if 0

std::string Schema::toJava (const std::string& name) {
	std::string res;
	if (table.size () == 0) return res;
	Table& tbl = table[0];
	for (std::vector<Table>::iterator it = table.begin (); it != table.end (); it++) {
		if (!name.compare (it->name)) {
			tbl = *it;
			break;
		}
	}

	if (name.compare (tbl.name)) return res;

	res += "package " + javaPackage + ";\nimport java.sql.*;\nimport java.util.*;\n";
	res += "class " + javaName (tbl.name) + " {\nConnection db;\npublic " + javaName (tbl.name) + " (Connection c) { db = c; " + tbl.pk + " = 0; deleted = false; }\n";
	res += "private boolean deleted;";
	std::string javatype;
	std::string javaname;
	std::string sqltype;
	std::string update;
	std::string updatesql;
	std::string insertsql1;
	std::string insertsql2;
	std::string insert;
	std::string loadsql;
	std::string load;
	std::string suffix;
	size_t i = 0, loadi = 1;
	for (std::vector<Field>::iterator it = tbl.field.begin (); it != tbl.field.end (); it++, loadi++, i++) {
		sqltype = sqlType (it->type);
		javatype = javaType (it->type);
		javaname = javaName (it->name);
		res += "private " + javatype + " " + javaname + ";\n";
		res += "public " + javatype + " get" + javaname + "() { return " + javaname + ";}\n";
		suffix = (!it->type.compare ("serial") ||!it->type.compare ("int") ? "Int" : (!it->type.compare ("boolean") ? "Boolean" : "String"));
		if (tbl.pk.compare (it->name)) {
			res += "public void set" + javaname + "(" + javatype + " __tmp) { " + javaname + " = __tmp;}\n";

			updatesql += it->name + " = ?,";
			update += (boost::format ("upd.set%1% (%2%, %3%);\n") % suffix % i % javaname).str ();

			insertsql1 += it->name + ",";
			insertsql2 += "?,";
			insert += (boost::format ("ins.set%1% (%2%, %3%);\n") % suffix % i % javaname).str ();
		}
		load += (boost::format ("%3% = rs.get%1% (%2%);\n") % suffix % loadi % javaname).str ();
		loadsql += it->name + ",";

	}

        res += "public int update () { if (!deleted) if (" + tbl.pk + " > 0) try {\n";
        res += "PreparedStatement upd = db.prepareStatement(\"UPDATE " + name + " SET " + updatesql.substr (0, updatesql.length () - 1) + " WHERE " + tbl.pk + " = ?\");\n";
        res += update;
        res += (boost::format ("upd.setInt (%1%, %2%);\n") % i % tbl.pk).str ();
        res += "upd.executeUpdate ();\nupdateList (); return " + tbl.pk + ";\n";
        res += "} catch (java.sql.SQLException e) {return 0;} else return insert (); return 0;}\n";

	res += "private int insert () {try {\n";
	res += "PreparedStatement ins = db.prepareStatement(\"INSERT INTO " + name + "(" + insertsql1.substr (0, insertsql1.length () - 1) + ") VALUES (" + insertsql2.substr (0, insertsql2.length () - 1) + ")\");\n";
	res += insert;
	res += "ins.executeUpdate ();\n";
	res += "PreparedStatement get = db.prepareStatement (\"SELECT currval('" + tbl.name + "_id_seq') \");\n";
	res += "ResultSet rs = get.executeQuery ();\n";
	res += tbl.pk + " = rs.getInt (1);\n";
	res += "updateList ();\n";
	res += "return " + tbl.pk + ";\n";
	res += "} catch (java.sql.SQLException e) {return 0;}}\n";

	res += "public void delete () {try {\n";
	res += "PreparedStatement del = db.prepareStatement(\"DELETE FROM " + name + " WHERE " + tbl.pk + " = ?\");\n";
	res += (boost::format ("del.setInt (1, %1%);\n") % tbl.pk).str ();
	res += "del.executeUpdate ();\ndeleted = true;\n";
	res += "deleteList ();\n";
	res += "} catch (java.sql.SQLException e) {}}\n";

	res += "public void load (int id) {try {\n";
	res += "PreparedStatement sel = db.prepareStatement(\"SELECT " + loadsql.substr (0, loadsql.length () - 1) + " FROM " + name + " WHERE " + tbl.pk + " = ?\");\n";
	res += (boost::format ("sel.setInt (1, %1%);\n") % tbl.pk).str ();
	res += "ResultSet rs = sel.executeQuery ();\n";
	res += load;
//	res += "loadext ();\n";
	res += "} catch (java.sql.SQLException e) {}}\n";

	std::string loadext = "PreparedStatement st; ResultSet rs; int foundNew = 0;\n";
	std::string updateList;

	// for each FK
	for (std::vector<FK>::iterator it = fk.begin (); it != fk.end (); it++) {
		if (!it->table1.compare (tbl.name)) {
			res += "private " + javaName (it->table2) + " _" + javaName (it->table2) + ";\n";
			res += "public " + javaName (it->table2) + " get" + javaName (it->table2) + "() { return _" + javaName (it->table2) + "; }\n";
			loadext += "_" + javaName (it->table2) + ".load (" + javaName (it->field1) + ");\n";
		}
		// search linked-only-tables
		if (!it->table2.compare (tbl.name)) {
			for (std::vector<FK>::iterator it2 = fk.begin (); it2 != fk.end (); it2++) {
				if (!it->table1.compare (it2->table1)) {
					if (it->table2.compare (it2->table2)) {
						res += "private List<" + javaName (it2->table2) + "> list" + javaName (it2->table2) + ";\n";
						res += "private List<" + javaName (it2->table2) + "> origlist" + javaName (it2->table2) + ";\n";
						res += "public List<" + javaName (it2->table2) + "> getList" + javaName (it2->table2) + " () { return list" + javaName (it2->table2) + "; }\n";

						loadext += "try { st = db.prepareStatement (\"SELECT id" + it2->table2 + " FROM " + it->table1 + " WHERE id" + tbl.name + " = ?\");\n";
						loadext += "list" + javaName (it2->table2) + " = new Vector<" + javaName (it2->table2) + ">();\n";
						loadext += "st.setInt (1, " + tbl.pk + ");\n";
						loadext += "rs = st.executeQuery ();\n";
						loadext += "if (rs != null) {\n";
						loadext += "while (rs.next ()) {\n";
						loadext += javaName (it2->table2) + " tmp = new " + javaName (it2->table2) + "(db);\n";
						loadext += "list" + javaName (it2->table2) + ".add (tmp);\n";
						loadext += "origlist" + javaName (it2->table2) + ".add (tmp);\n";
						loadext += "}\n";
						loadext += "}\n} catch (java.sql.SQLException e) {}";

						updateList += "foundNew = 0;";
						updateList += "for (int i = 0; i < list" + javaName (it2->table2) + ".size (); i++) {\n";
						updateList += "for (int j = 0; j < origlist" + javaName (it2->table2) + ".size (); j++) {\n";
						updateList += "if (list" + javaName (it2->table2) + ".get (i).getid () != origlist" + javaName (it2->table2) + ".get (j).getid ()) { if (foundNew != 2) foundNew = 1; } else {foundNew = 2;}";
						updateList += "}\n";
						updateList += "if (foundNew != 2) {\n";
						updateList += "int tmp = list" + javaName (it2->table2) + ".get(i).update ();\n";
						updateList += javaName (it->table1) + " tmp2 = new " + javaName (it->table1) + "(db);\n";
						updateList += "tmp2.set" + javaName (it->field1) + "(" + tbl.pk + ");\n";
						updateList += "tmp2.set" + javaName (it2->field1) + "(tmp);\n";
						updateList += "tmp2.update ();\n";
						updateList += "} else list" + javaName (it2->table2) + ".get(i).update ();\n";
						updateList += "}\n";

						updateList += "try { for (int j = 0; j < origlist" + javaName (it2->table2) + ".size (); j++) {\n";
						updateList += "for (int i = 0; i < list" + javaName (it2->table2) + ".size (); i++) {\n";
						updateList += "if (list" + javaName (it2->table2) + ".get(i).getid () != origlist" + javaName (it2->table2) + ".get(j).getid ()) { if (foundNew != 2) foundNew = 1; } else {foundNew = 2;}";
						updateList += "if (foundNew != 2) {\n";
						updateList += "st = db.prepareStatement (\"DELETE FROM " + it->table1 + " WHERE " + it->field1 + " = ? AND " + it2->field1 + " = ?\");\n";
						updateList += "st.setInt (1, " + tbl.pk + ");\n";
						updateList += "st.setInt (1, origlist" + javaName (it2->table2) + ".get(j).getid ());\n";
						updateList += "st.executeQuery ();\n";
						updateList += "}\n";
						updateList += "}\n";
						updateList += "}\n} catch (java.sql.SQLException e) {}";
					}
				}
			}
		}
	}

	res += "public void loadext () {\n" + loadext + "\n}\n";

	/*if (updateList.length ()) */res += "private void updateList () { PreparedStatement st; ResultSet rs; int foundNew = 0;\n" + updateList + "\n}\n";
//	else res += "private void updateList () {}\n";

	res += "private void deleteList () {}\n";

	res += "}";

	return res;
}

#endif

std::string Schema::toHbm (const std::string& name) {
	std::string res;
        if (table.size () == 0) return res;
        Table& tbl = table[0];
        for (std::vector<Table>::iterator it = table.begin (); it != table.end (); it++) {
                if (!name.compare (it->name)) {
                        tbl = *it;
                        break;
                }
        }

        if (name.compare (tbl.name)) return res;

	res += "<?xml version=\"1.0\"?>\n<!DOCTYPE hibernate-mapping\n\tPUBLIC\n\t\"-//Hibernate/Hibernate Mapping DTD 3.0//EN\"\n\t\"http://hibernate.sourceforge.net/hibernate-mapping-3.0.dtd\">\n<hibernate-mapping><class name=\"";
	res += javaName (name);
	res += "\">\n<id name=\"id\" type=\"int\" column=\"id\" unsaved-value=\"null\"><generator class=\"sequence\"><param name=\"sequence\">" + name + "_id_seq</param></generator></id>\n";
	
	for (std::vector<Field>::iterator it = tbl.field.begin (); it != tbl.field.end (); it++) {
		if (it->name.compare ("id")) res += "<property name=\"" + javaName (it->name) + "\" type=\"" + javaType (it->type) + "\"><column name=\"" + sqlType (it->name) + "\"/></property>\n";
	}

	std::string suffix = "";
	// for each FK
        for (std::vector<FK>::iterator it = fk.begin (); it != fk.end (); it++) {
		if (!it->table1.compare (tbl.name)) {
			res += "<many-to-one name=\"m_" + javaName (it->table2) + suffix + "\" class=\"" + javaName (it->table2) + "\" ><column name=\"" + javaName (it->field1) + "\" not-null=\"false\" /></many-to-one>\n";
			suffix += "1";
		} else if (!it->table2.compare (tbl.name)) {
			res += "<set name=\"" + javaName (it->table1) + "s" + suffix + "\" inverse=\"true\" cascade=\"none\"><key><column name=\"" + javaName (it->field1) + "\" not-null=\"true\" /></key><one-to-many class=\"" + javaName (it->table1) + "\"/></set>\n";
			suffix += "1";
		}
	}
	res += "</class>\n</hibernate-mapping>\n";
	return res;
}

std::vector<std::string> Schema::listTables () const {
	std::vector<std::string> res;
	for (std::vector<Table>::const_iterator it = table.begin (); it != table.end (); it++) {
		res.push_back (it->name);
	}
	return res;
}

std::vector<std::string> Schema::listClasses () const {
        std::vector<std::string> res;
        for (std::vector<Table>::const_iterator it = table.begin (); it != table.end (); it++) {
                if (!it->isLink) res.push_back (javaName (it->name));
        }
        return res;
}


