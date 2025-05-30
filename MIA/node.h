#ifndef NODE_H
#define NODE_H

#include <nlohmann/json.hpp>

class Node {
	public:
		virtual bool push(nlohmann::json data) = 0;
		nlohmann::json pull();

	private:
		nlohmann::json m_data_in;
		nlohmann::json m_data_out;
};

#endif
