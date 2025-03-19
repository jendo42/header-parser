#include <unordered_set>
#include <stack>
#include <cassert>
#include <sstream>

#include "Node.h"

static std::string dotEscape(const std::string& str)
{
	std::string out;
	for (auto c : str) {
		switch (c) {
		case '\'':
			out.append("&#39;");
			continue;
		case '\\':
			out.append("&#92;");
			continue;
		case '\r':
			continue;
		case '\n':
			out.append("<br align=\"left\" />");
			continue;
		case '\t':
			out.append("    ");
			continue;
		case '>':
			out.append("&#62;");
			continue;
		case '<':
			out.append("&#60;");
			continue;
		case '"':
			out.append("&#34;");
			continue;
		case '&':
			out.append("&amp;");
			continue;
		}
		out.push_back(c);
	}
	return out;
}
/*
std::string Node::makeDot(Token errorToken, Token prevToken, ParserError err)
{
	size_t errorNodeId = 33;
	std::string out;
	out.append("graph G {\nnode [margin=0.1 fontcolor=black fillcolor=darkolivegreen1 fontsize=4 shape=box style=filled fontname=monospace]\n");
	visit(nullptr, [&](Mavis::Script::Node* node) {
		std::ostringstream error;
		bool errorNode = false;
		auto index = node->token().lineOffset(token().m_start, strlen(token().m_start));
		out.append(std::to_string((size_t)node));
		out.append(" [label=<<font point-size=\"5\"><b>@");
		out.append(std::to_string(index.first));
		out.append(":");
		out.append(std::to_string(index.second + 1));
		out.append("<br/>");
		out.append(node->token().getTypeName());
		out.append("</b></font><br/><br/>");
		out.append(dotEscape(node->token().toString()));
		out.append("<br align=\"left\" />>");
		if (node->error() != ParserError::Ok) {
			auto sourceStart = node->token().m_start;
			auto sourceLen = strlen(node->token().m_start);
			auto errorIndex = node->token().lineOffset(sourceStart, sourceLen);
			error << '@' << errorIndex.first << ':' << errorIndex.second + 1 << "\nParser error near " << node->token().getTypeName() << " '" << node->token().toString() << "', " << ErrorMessage(err);
			if (err == ParserError::UnexpectedToken) {
				error << ": " << errorToken.getTypeName() << " '" << errorToken.toString() << "'";
			}
			error << "\n\n" << Lexer::GetLine(errorIndex.first, sourceStart, sourceLen).toString() << "\n";
			error << std::string(errorIndex.second, '~') << "^\n";
			out.append(" fillcolor=firebrick1");
			errorNode = true;
		}
		out.append("]\n");
		if (errorNode) {
			out.append(std::to_string(errorNodeId));
			out.append(" [label=<<font point-size=\"5\"><b>");
			out.append(dotEscape(error.str()));
			out.append("</b></font>> shape=oval color=firebrick1 fontcolor=firebrick1 style=dotted]\n");
			out.append(std::to_string((size_t)node));
			out.append(" -- ");
			out.append(std::to_string(errorNodeId));
			out.append(" [style = dotted color = firebrick1]\n");
			++errorNodeId;
		}
		return true;
	});
	visit(nullptr, [&](Mavis::Script::Node* node) {
		if (node->parent()) {
			out.append(std::to_string((size_t)node->parent()));
			out.append(" -- ");
			out.append(std::to_string((size_t)node));
			out.append("\n");
		}
		return true;
	});
	out.append("}\n");
	return out;
}
*/
