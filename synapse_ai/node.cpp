#include "node.h"

nlohmann::json Node::pull() {
	return m_data_out;
}
