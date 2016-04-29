/*
 * Taken and adapted from Ben Strasser, Karlsruhe Institute of Technology (KIT),
 * Institute of Theoretical Informatics, Algorithmics I.
 *
 * Copyright (c) 2016 Michael Wegner and Matthias Wolf
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef VECTOR_IO_H
#define VECTOR_IO_H

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>

template<class T>
void save_vector(const std::string&file_name, const std::vector<T>&vec){
	std::ofstream out(file_name, std::ios::binary);
	if(!out)
		throw std::runtime_error("Can not open \""+file_name+"\" for writing.");
	out.write(reinterpret_cast<const char*>(&vec[0]), vec.size()*sizeof(T));
}

template<class T>
std::vector<T>load_vector(const std::string&file_name){
	std::ifstream in(file_name, std::ios::binary);
	if(!in)
		throw std::runtime_error("Can not open \""+file_name+"\" for reading.");
	in.seekg(0, std::ios::end);
	unsigned long long file_size = in.tellg();
	if(file_size % sizeof(T) != 0)
		throw std::runtime_error("File \""+file_name+"\" can not be a vector of the requested type because it's size is no multiple of the element type's size.");
	in.seekg(0, std::ios::beg);
	std::vector<T>vec(file_size / sizeof(T));
	in.read(reinterpret_cast<char*>(&vec[0]), file_size);
	return vec; // NVRO
}

template<>
inline void save_vector<std::string>(const std::string&file_name, const std::vector<std::string>&vec){
	std::ofstream out(file_name, std::ios::binary);
	for(unsigned i=0; i<vec.size(); ++i){
		const char*x = vec[i].c_str();
		out.write(x, vec[i].length()+1);
	}
}

template<>
inline std::vector<std::string>load_vector<std::string>(const std::string&file_name){
	std::vector<char>data = load_vector<char>(file_name);
	std::vector<std::string>ret;
	std::vector<char>::const_iterator 
		str_begin = data.begin(), 
		str_end = data.begin(), 
		data_end = data.end();

	while(str_end != data_end){
		if(*str_end == '\0'){
			ret.push_back(std::string(str_begin, str_end));
			++str_end;
			str_begin = str_end;
		}else{
			++str_end;
		}
	}

	ret.shrink_to_fit();
	return ret; // NVRO
}


#endif
