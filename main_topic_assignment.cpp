#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <sstream>
#include <cstdlib>
#include <cstdio>
#include <iomanip>
#include "libs/cxxopts/cxxopts.hpp"

std::vector<std::pair<std::string, std::size_t>> read_in_topics(std::istream& istr)
{
	std::vector<std::pair<std::string, std::size_t>> topics;
	std::string topic_line;
	while(std::getline(istr, topic_line))
	{
		auto x_pos = topic_line.find_first_of("x");
		std::string num = topic_line.substr(0, x_pos);
		std::string title = topic_line.substr(x_pos+1);
		if(title.empty())
			continue;

		topics.push_back({ title, std::stoul(num) });
	}

	return topics;
}


std::vector<std::pair<std::string, std::vector<unsigned>>> read_in_student_preferences(std::istream& istr)
{
	// read in student preferences
	std::map<std::string, std::vector<unsigned>> student_preferences;
	std::string line;
	std::size_t i_line = 1;
	while(std::getline(istr, line))
	{
		std::stringstream ss(line);
		std::string student_name;
		std::vector<unsigned> student_topic_preferences;
		ss >> student_name;	
		unsigned preference;
		while(!ss.eof())
		{
			ss >> preference;
			if(preference == 0)
			{
				throw std::logic_error("Parsing error in student preference file at line " +  std::to_string(i_line));
			}
			student_topic_preferences.push_back(preference);
		}

		student_preferences.insert({ student_name, student_topic_preferences });

		i_line++;
	}

	return std::vector<std::pair<std::string, std::vector<unsigned>>>(student_preferences.begin(), student_preferences.end());
}

std::vector<unsigned> read_in_weights(std::istream& istr)
{
	std::vector<unsigned> weights;
	while(!istr.eof())
	{
		unsigned weight;	
		istr >> weight;
		weights.push_back(weight);
	}

	return weights;
}

int main(int argc, const char** argv)
{

    cxxopts::Options options("Student topic assignment tool", "Student topic assignment tool");
	options.add_options()
		("h,help", "Produce help message.")
		("t,topics", "File in which each line contains the name of a topic. The line always has to start with a multiplicity, e.g. 1x, 2x, ...", cxxopts::value<std::string>())
		("p,preferences", "File in which each line contains a student. Lines start with a name of the student (without spaces) followed by the topic ids (start at 1 for the first topic) ordered by preference.", cxxopts::value<std::string>())
		("w,weights", "File in which line n contains one single number specifying how much weight is put on the weight is put on n-th choice of a student.", cxxopts::value<std::string>())
		("r,show-results", "Assignments of students to topics are only shown if this flag is set.")
	;
	auto args = options.parse(argc, argv);

	if (args.count("help")) {
		std::cout << options.help() << std::endl;
		return 0;
	}

	// check if all parameters were specified
	if(args.count("topics") == 0 || args.count("preferences") == 0 || args.count("weights") == 0)
	{
		std::cerr << "Please provide --topics, --preferences, and --weights." << std::endl;
		return 1;
	}

	// open files
	std::ifstream topics_file(args["topics"].as<std::string>()), student_preferences_file(args["preferences"].as<std::string>()), weight_file(args["weights"].as<std::string>());

	if(!topics_file.is_open()) {
		std::cerr << "Topics file could not be opened." << std::endl;
		return 1;
	}
	if(!student_preferences_file.is_open()) {
		std::cerr << "Preferences file could not be opened." << std::endl;
		return 1;
	}
	if(!weight_file.is_open()) {
		std::cerr << "Weights file could not be opened." << std::endl;
		return 1;
	}

	// read in data
	auto topics = read_in_topics(topics_file);
	auto student_preferences = read_in_student_preferences(student_preferences_file);
	auto weights = read_in_weights(weight_file);

	std::size_t n_students = student_preferences.size();
	std::size_t n_distinct_topics = topics.size();
	std::size_t n_topics = 0;
	for(auto topic : topics)
		n_topics += topic.second;

	if(n_students > n_topics)
		throw std::logic_error("Number of all topics (with potential duplicates) has to be larger than number of students.");

	// build cost matrix
	std::vector<std::vector<unsigned>> c_matrix(student_preferences.size(), std::vector<unsigned>(n_distinct_topics, 0));
	std::size_t i_student = 0;
	for(auto student_preference : student_preferences)
	{
		for(std::size_t i_pref = 0; i_pref < student_preference.second.size(); i_pref++)
		{
			c_matrix[i_student][student_preference.second[i_pref]-1] = weights[i_pref];
		}

		i_student++;
	}

	// build lp file
	std::ofstream topic_assignment_lp_file("topic_assignment.lp");
	topic_assignment_lp_file << "/* Generated by ./topic_assignment */" << std::endl << std::endl;

	// build target function
	topic_assignment_lp_file << "max: ";
	bool first = true;
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
		{
			if(c_matrix[i_student][i_topic] == 0)
				continue;

			if(first)
				first = false;
			else 
				topic_assignment_lp_file << " + ";

			topic_assignment_lp_file << c_matrix[i_student][i_topic] << " " << "x_" << i_student << "_" << i_topic;
		}
	topic_assignment_lp_file << ";" << std::endl << std::endl;

	// build constraint that each topic is only picked as often as specified
	topic_assignment_lp_file << "/* build constraint that each topic is only picked as often as specified */" << std::endl << std::endl;
	for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
	{
		first = true;
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
		{
			if(first)
				first = false;
			else 
				topic_assignment_lp_file << " + ";

			topic_assignment_lp_file <<  "x_" << i_student << "_" << i_topic;
		}
		topic_assignment_lp_file <<  " <= " << topics[i_topic].second << ";" << std::endl;
	}
	topic_assignment_lp_file << std::endl;

	// build constraint that each student only picks one topic
	topic_assignment_lp_file << "/* build constraint that each student only picks one topic */" << std::endl << std::endl;
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
	{
		first = true;
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
		{
			if(first)
				first = false;
			else 
				topic_assignment_lp_file << " + ";

			topic_assignment_lp_file <<  "x_" << i_student << "_" << i_topic;
		}
		topic_assignment_lp_file <<  " = 1;" << std::endl;
	}
	topic_assignment_lp_file << std::endl;

	for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
			topic_assignment_lp_file <<  "bin x_" << i_student << "_" << i_topic << ";" << std::endl;


	// run lp_solve on problem instance and save output to file
	std::system("lp_solve topic_assignment.lp | tail -n +5 > assignment_out.txt");

	// open output file and read it
	std::ifstream result_file("assignment_out.txt");
	std::vector<std::vector<unsigned>> result_matrix(n_students, std::vector<unsigned>(n_distinct_topics, 0));
	std::string line;
	while(std::getline(result_file, line))
	{
		std::stringstream ss(line);
		std::string variable, value;
		ss >> variable >> value;

		std::stringstream ss2(variable);
		std::string i_student_str, i_topic_str, dump;
		std::getline(ss2, dump, '_');
		std::getline(ss2, i_student_str, '_');
		std::getline(ss2, i_topic_str, '_');

		std::size_t i_student = std::stoul(i_student_str);
		std::size_t i_topic = std::stoul(i_topic_str);

		if(value == "0")
			result_matrix[i_student][i_topic] = 0;
		else
			result_matrix[i_student][i_topic] = 1;
	}

	// build map that assigns student number
	std::map<std::size_t,std::size_t> student_to_topic;
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
			if(result_matrix[i_student][i_topic])
				student_to_topic.insert({ i_student, i_topic });

	// compute number of students that did not get any of their priorities
	std::size_t n_non_priorities = 0;
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
	{
		bool has_prio = false;
		for(auto p : student_preferences[i_student].second)
			if(p == student_to_topic[i_student] + 1)
				has_prio = true;

		if(!has_prio)
			n_non_priorities++;
	}

	// check consistency of result_matrix (every student has exactly one topic)
	for(std::size_t i_student = 0; i_student < n_students; i_student++)
	{
		std::size_t n_trues = 0;
		for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
			n_trues += (result_matrix[i_student][i_topic]) ? 1 : 0;

		if(n_trues != 1)
			throw std::logic_error("Error in the output of lp_solve: Student " + student_preferences[i_student].first + " should have exactly one topic but has " + std::to_string(n_trues) + ".");
	}

	// check consistency of result_matrix (every topic is picked by exactly one student)
	for(std::size_t i_topic = 0; i_topic < n_distinct_topics; i_topic++)
	{
		std::size_t n_trues = 0;
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
			n_trues += (result_matrix[i_student][i_topic]) ? 1 : 0;

		if(n_trues > topics[i_topic].second)
			throw std::logic_error("Error in the output of lp_solve: Topic " + topics[i_topic].first + " should have been picked exactly " + std::to_string(topics[i_topic].second) + " times but has been picked " + std::to_string(n_trues) + " times.");
	}

	// print results
	std::cout << "Students that did not get any of their priorities: " << n_non_priorities << std::endl;

	if(args.count("show-results"))
	{
		for(std::size_t i_student = 0; i_student < n_students; i_student++)
			std::cout <<  std::left << std::setw(20) << student_preferences[i_student].first << " --->    " << topics[student_to_topic[i_student]].first << std::endl;
	}

	// delete files that were created
	std::remove("topic_assignment.lp");
	std::remove("assignment_out.txt");

	return 0;
}
