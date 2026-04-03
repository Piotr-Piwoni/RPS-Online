#pragma once
#include <chrono>
#include <ctime>
#include <format>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>

namespace Game
{
enum MessageType : uint8_t { INFO, DEBUG, WARNING, ERROR, SUCCESS };

enum SourceType : uint8_t { SERVER, CLIENT };

class Log
{
public:
	/// @brief Outputs a fully formatted string.
	///
	/// This function formats a string by replating placeholder values "{}" with
	/// the actual arguments.
	///
	/// @param fmt  The message to format.
	/// @param args The argument to put in the formatted string.
	/// @return The fully formatted string.
	template<typename... Args>
	static std::string Format(const std::format_string<Args...> fmt,
							  Args&&... args)
	{
		return std::format(fmt, std::forward<Args>(args)...);
	}

	/// @brief Prints a timestamped message to the console with colour coding.
	///
	/// This function outputs a message prefixed by the current local time,
	/// formatted as "DD-MM-YYYY HH:MM:SS". The message is colour-coded
	/// according to its severity level:
	/// - DEBUG → Cyan
	/// - WARNING → Brown
	/// - ERROR → Red (bold)
	/// - SUCCESS → Green (bold)
	/// - INFO (default) → Plain Text
	///
	/// @param msg  The message to print.
	/// @param mType The message category/severity. Defaults to INFO.
	/// @param sType The source of the message. Defaults to CLIENT.
	/// @param terminateOnError A flag for whether to terminate the application
	/// if an error occurs. Defaults to FALSE.
	static void PrintMsg(std::string_view msg,
						 const MessageType mType = INFO,
						 const SourceType sType = CLIENT,
						 const bool terminateOnError = false)
	{
		// Lock this thread for console output.
		static std::mutex coutMutex;
		std::lock_guard lock(coutMutex);

		const auto now = std::chrono::system_clock::now();
		const std::time_t currentTime =
				std::chrono::system_clock::to_time_t(now);
		std::tm localTime = {};
		// Cast to VOID to ignore return.
		static_cast<void>(localtime_s(&localTime, &currentTime));

		// Extract microseconds.
		const auto micro = std::chrono::duration_cast<
							   std::chrono::microseconds>(
								   now.time_since_epoch()) % 1'000'000;

		std::ostringstream timestamp;
		timestamp << std::put_time(&localTime, "%d-%m-%Y %H:%M:%S")
				<< ':' << std::setfill('0') << std::setw(6) << micro.count();

		// Source header.
		std::string sourceText;
		switch (sType)
		{
		case SERVER:
			sourceText = "Server";
			break;
		case CLIENT:
			sourceText = "Client";
			break;
		}

		// Severity colour.
		std::string severityColour;
		switch (mType)
		{
		case DEBUG:
			severityColour = "\033[36m";
			break;
		case WARNING:
			severityColour = "\033[33m";
			break;
		case ERROR:
			severityColour = "\033[1;31m";
			break;
		case SUCCESS:
			severityColour = "\033[1;32m";
			break;
		case INFO:
			severityColour = "\033[0m";
			break;
		}

		const std::string gold = "\033[38;5;220m";
		const std::string reset = "\033[0m";

		// Print message.
		std::cout << "[" << gold << std::setw(sourceText.size()) << std::left
				<< sourceText << reset << "]  "
				<< severityColour
				<< std::setw(timestamp.str().size()) << std::left
				<< timestamp.str() << "    "
				<< msg << reset << '\n';

		// If the terminate flag is set, and it's an ERROR, stop the program.
		if (terminateOnError && mType == ERROR)
			throw std::runtime_error(msg.data());
	}
};
}
