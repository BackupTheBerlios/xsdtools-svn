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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <ostream>
#include <fstream>
#include <getopt.h>
#include <string.h>
#include <utility>
#include <sys/stat.h>
#include <sys/types.h>

#include "schema.h"

static const char* const short_options = "hx:i:sl:b:p:";
static const struct option long_options [] = {
        {"help", 0, NULL, 'h'},
        {"xsd", 1, NULL, 'x'},
	{"individual", 1, NULL, 'i'},
	{"list", 0, NULL, 'l'},
	{"hbm", 1, NULL, 'b'},
	{"package", 0, NULL, 'p'},
        {NULL, 0, NULL, 0}
};

static void display_usage () {
        std::cout << "--help           This help" << std::endl;
        std::cout << "--xsd, -x        Point to the xsd-file" << std::endl;
	std::cout << "--individual, -i Generate individual table for pointed entity" << std::endl;
	std::cout << "--list, -l       List of classes" << std::endl;
        std::cout << "--hbm, -b        Generate .hbm.xml files for pointed table" << std::endl;
	std::cout << "--package, -p    Name of java's package" << std::endl;
}

int main (int argc, char** argv) {
	std::string file;
	std::string out;
	std::vector<std::string> individual;

	bool listClasses = false;
	std::string hbm;
	bool package = false;

        int c;
        while ((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1) {
                switch (c) {
                        case 'h':
                                display_usage ();
                                exit (0);
                        break;
                        case 'x':
                                if (optarg) file = optarg;
                        break;
			case 'i':
				if (optarg) individual.push_back (optarg);
			break;
			case 'l':
				listClasses = true;
			break;
			case 'b':
				if (optarg) hbm = optarg;
			break;
			case 'p':
				package = true;
			break;
                }
        }

	if (!file.length ()) {
		display_usage ();
		return 0;
	}

	Schema sch;
	sch.load (file, individual);

	if (listClasses) {
		std::vector<std::string> table = sch.listTables ();
		for (size_t i = 0; i < table.size (); i++) {
			std::cout << table[i] << " " << lang::javaName (table[i]) << std::endl;
		}
		return 0;
	}

        if (hbm.length ()) {
                std::cout << sch.toHbm (hbm) << std::endl;
                return 0;
        }

	if (package) {
		std::cout << sch.javaPackage << std::endl;
		return 0;
	}

	return 0;
}

