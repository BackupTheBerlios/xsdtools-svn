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

#ifndef __COMPLEX_TYPE__
#define __COMPLEX_TYPE__

#include "element.h"
#include <vector>
#include <boost/shared_ptr.hpp>

typedef boost::shared_ptr<Element> pElement;

class ComplexType {
public:
	std::string name;
	std::vector<Element> sequence;
};

#endif

