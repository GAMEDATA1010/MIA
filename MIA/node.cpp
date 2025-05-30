virtual bool Node::push(nlohmann::json data) = 0;

nlohmann::json Node::pull() {
	return m_data_out;
}
