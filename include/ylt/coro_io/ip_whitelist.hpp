/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "asio/ip/address.hpp"
#include "asio/ip/network_v4.hpp"
#include "asio/ip/network_v6.hpp"
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>
#include <regex>
#include <cstring>
#include <sstream>
#include "ylt/easylog.hpp"

namespace coro_io {

/**
 * @brief IP Whitelist Manager
 *
 * Provides IP whitelist management functionality, supporting:
 * - Single IP addresses
 * - CIDR networks
 * - IP address ranges
 * - Regular expression matching
 */
class ip_whitelist {
public:
    /**
     * @brief Default constructor, creates empty whitelist
     */
    ip_whitelist() = default;
    
    /**
     * @brief Copy constructor
     * @param other Source object to copy from
     */
    ip_whitelist(const ip_whitelist& other) {
        std::shared_lock<std::shared_mutex> lock(other.mutex_);
        single_ips_ = other.single_ips_;
        cidr_v4_networks_ = other.cidr_v4_networks_;
        cidr_v6_networks_ = other.cidr_v6_networks_;
        ip_ranges_ = other.ip_ranges_;
        regex_patterns_ = other.regex_patterns_;
    }
    
    /**
     * @brief Copy assignment operator
     * @param other Source object to copy from
     * @return Reference to current object
     */
    ip_whitelist& operator=(const ip_whitelist& other) {
        if (this != &other) {
            std::shared_lock<std::shared_mutex> other_lock(other.mutex_);
            std::unique_lock<std::shared_mutex> this_lock(mutex_);
            
            single_ips_ = other.single_ips_;
            cidr_v4_networks_ = other.cidr_v4_networks_;
            cidr_v6_networks_ = other.cidr_v6_networks_;
            ip_ranges_ = other.ip_ranges_;
            regex_patterns_ = other.regex_patterns_;
        }
        return *this;
    }
    
    /**
     * @brief Move constructor
     * @param other Source object to move from
     */
    ip_whitelist(ip_whitelist&& other) noexcept {
        std::unique_lock<std::shared_mutex> lock(other.mutex_);
        single_ips_ = std::move(other.single_ips_);
        cidr_v4_networks_ = std::move(other.cidr_v4_networks_);
        cidr_v6_networks_ = std::move(other.cidr_v6_networks_);
        ip_ranges_ = std::move(other.ip_ranges_);
        regex_patterns_ = std::move(other.regex_patterns_);
    }
    
    /**
     * @brief Move assignment operator
     * @param other Source object to move from
     * @return Reference to current object
     */
    ip_whitelist& operator=(ip_whitelist&& other) noexcept {
        if (this != &other) {
            std::unique_lock<std::shared_mutex> other_lock(other.mutex_);
            std::unique_lock<std::shared_mutex> this_lock(mutex_);
            
            single_ips_ = std::move(other.single_ips_);
            cidr_v4_networks_ = std::move(other.cidr_v4_networks_);
            cidr_v6_networks_ = std::move(other.cidr_v6_networks_);
            ip_ranges_ = std::move(other.ip_ranges_);
            regex_patterns_ = std::move(other.regex_patterns_);
        }
        return *this;
    }
    
    /**
     * @brief Constructor, initialize with IP address list
     * @param ips List of IP addresses
     */
    explicit ip_whitelist(const std::vector<std::string>& ips) {
        for (const auto& ip : ips) {
            add_ip(ip);
        }
    }

    /**
     * @brief Add single IP address to whitelist
     * @param ip_str IP address string, supports IPv4/IPv6
     * @return true if successfully added, false if format error
     */
    bool add_ip(const std::string& ip_str) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        try {
            auto addr = asio::ip::address::from_string(ip_str);
            single_ips_.insert(addr);
            ELOG_INFO << "Added IP to whitelist: " << ip_str;
            return true;
        } catch (const std::exception& e) {
            ELOG_ERROR << "Failed to add IP to whitelist: " << ip_str 
                      << ", error: " << e.what();
            return false;
        }
    }

    /**
     * @brief Add CIDR network to whitelist
     * @param cidr_str CIDR network string, e.g. "192.168.1.0/24"
     * @return true if successfully added, false if format error
     */
    bool add_cidr(const std::string& cidr_str) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        try {
            // Separate IP and prefix length
            size_t slash_pos = cidr_str.find('/');
            if (slash_pos == std::string::npos) {
                return false;
            }
            
            std::string ip_part = cidr_str.substr(0, slash_pos);
            std::string prefix_part = cidr_str.substr(slash_pos + 1);
            
            auto addr = asio::ip::address::from_string(ip_part);
            int prefix_length = std::stoi(prefix_part);
            
            if (addr.is_v4()) {
                auto network = asio::ip::make_network_v4(cidr_str);
                cidr_v4_networks_.push_back(network);
            } else {
                auto network = asio::ip::make_network_v6(cidr_str);
                cidr_v6_networks_.push_back(network);
            }
            
            ELOG_INFO << "Added CIDR to whitelist: " << cidr_str;
            return true;
        } catch (const std::exception& e) {
            ELOG_ERROR << "Failed to add CIDR to whitelist: " << cidr_str 
                      << ", error: " << e.what();
            return false;
        }
    }

    /**
     * @brief Add IP address range to whitelist
     * @param start_ip Starting IP address
     * @param end_ip Ending IP address
     * @return true if successfully added, false if format error
     */
    bool add_ip_range(const std::string& start_ip, const std::string& end_ip) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        try {
            auto start_addr = asio::ip::address::from_string(start_ip);
            auto end_addr = asio::ip::address::from_string(end_ip);
            
            if (start_addr.is_v4() != end_addr.is_v4()) {
                ELOG_ERROR << "IP version mismatch in range: " << start_ip << " - " << end_ip;
                return false;
            }
            
            ip_ranges_.emplace_back(start_addr, end_addr);
            ELOG_INFO << "Added IP range to whitelist: " << start_ip << " - " << end_ip;
            return true;
        } catch (const std::exception& e) {
            ELOG_ERROR << "Failed to add IP range to whitelist: " << start_ip 
                      << " - " << end_ip << ", error: " << e.what();
            return false;
        }
    }

    /**
     * @brief Add regex pattern to whitelist
     * @param pattern Regular expression pattern string
     * @return true if successfully added, false if format error
     */
    bool add_regex_pattern(const std::string& pattern) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        try {
            regex_patterns_.emplace_back(pattern);
            ELOG_INFO << "Added regex pattern to whitelist: " << pattern;
            return true;
        } catch (const std::exception& e) {
            ELOG_ERROR << "Failed to add regex pattern to whitelist: " << pattern 
                      << ", error: " << e.what();
            return false;
        }
    }

    /**
     * @brief Universal add method - automatically detects format and adds to whitelist
     * @param entry String entry that can be: single IP, CIDR network, IP range, or regex pattern
     * @return true if successfully added, false if format error
     *
     * Format detection:
     * - Contains "/" -> CIDR network (e.g., "192.168.1.0/24")
     * - Contains " - " -> IP range (e.g., "192.168.1.1 - 192.168.1.100")
     * - Contains regex special chars -> Regex pattern (e.g., "192\.168\.1\.[0-9]+")
     * - Otherwise -> Single IP address (e.g., "192.168.1.1")
     */
    bool add(const std::string& entry) {
        if (entry.empty()) {
            ELOG_ERROR << "Empty entry provided to add()";
            return false;
        }
        
        // Check for CIDR format (contains "/")
        if (entry.find('/') != std::string::npos) {
            return add_cidr(entry);
        }
        
        // Check for IP range format (contains " - ")
        size_t dash_pos = entry.find(" - ");
        if (dash_pos != std::string::npos) {
            std::string start_ip = entry.substr(0, dash_pos);
            std::string end_ip = entry.substr(dash_pos + 3);
            return add_ip_range(start_ip, end_ip);
        }
        
        // Check for regex pattern (contains regex special characters)
        // Common regex special chars: . * + ? ^ $ ( ) [ ] { } | backslash
        {
            bool has_regex_char = false;
            const std::string regex_chars = ".*+?^$()[]{}|\\";
            for (char c : entry) {
                if (regex_chars.find(c) != std::string::npos) {
                    has_regex_char = true;
                    break;
                }
            }
            
            if (has_regex_char) {
                return add_regex_pattern(entry);
            }
        }
        
        // Default: treat as single IP address
        return add_ip(entry);
    }

    /**
     * @brief Remove IP address from whitelist
     * @param ip_str IP address string
     * @return true if successfully removed
     */
    bool remove_ip(const std::string& ip_str) {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        try {
            auto addr = asio::ip::address::from_string(ip_str);
            auto count = single_ips_.erase(addr);
            if (count > 0) {
                ELOG_INFO << "Removed IP from whitelist: " << ip_str;
                return true;
            }
            return false;
        } catch (const std::exception& e) {
            ELOG_ERROR << "Failed to remove IP from whitelist: " << ip_str 
                      << ", error: " << e.what();
            return false;
        }
    }

    /**
     * @brief Clear whitelist
     */
    void clear() {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        single_ips_.clear();
        cidr_v4_networks_.clear();
        cidr_v6_networks_.clear();
        ip_ranges_.clear();
        regex_patterns_.clear();
        ELOG_INFO << "Cleared IP whitelist";
    }

    /**
     * @brief Check if IP address is in whitelist
     * @param ip_addr IP address object
     * @return true if in whitelist, false otherwise
     */
    bool is_allowed(const asio::ip::address& ip_addr) const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        
        // Check single IPs
        if (single_ips_.find(ip_addr) != single_ips_.end()) {
            return true;
        }

        // Check CIDR networks
        if (ip_addr.is_v4()) {
            auto v4_addr = ip_addr.to_v4();
            for (const auto& network : cidr_v4_networks_) {
                if (network.hosts().find(v4_addr) != network.hosts().end()) {
                    return true;
                }
            }
        } else {
            auto v6_addr = ip_addr.to_v6();
            for (const auto& network : cidr_v6_networks_) {
                if (network.hosts().find(v6_addr) != network.hosts().end()) {
                    return true;
                }
            }
        }

        // Check IP ranges
        for (const auto& range : ip_ranges_) {
            if (is_ip_in_range(ip_addr, range.first, range.second)) {
                return true;
            }
        }

        // Check regex patterns
        std::string ip_str = ip_addr.to_string();
        for (const auto& pattern : regex_patterns_) {
            if (std::regex_match(ip_str, pattern)) {
                return true;
            }
        }

        return false;
    }

    /**
     * @brief Check if IP address string is in whitelist
     * @param ip_str IP address string
     * @return true if in whitelist, false if format error or not in whitelist
     */
    bool is_allowed(const std::string& ip_str) const {
        try {
            auto addr = asio::ip::address::from_string(ip_str);
            return is_allowed(addr);
        } catch (const std::exception& e) {
            ELOG_ERROR << "Invalid IP address format: " << ip_str 
                      << ", error: " << e.what();
            return false;
        }
    }

    /**
     * @brief Get whitelist size
     * @return Number of entries in whitelist
     */
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return single_ips_.size() + cidr_v4_networks_.size() + 
               cidr_v6_networks_.size() + ip_ranges_.size() + regex_patterns_.size();
    }

    /**
     * @brief Check if whitelist is empty
     * @return true if empty, false otherwise
     */
    bool empty() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return single_ips_.empty() && cidr_v4_networks_.empty() && 
               cidr_v6_networks_.empty() && ip_ranges_.empty() && regex_patterns_.empty();
    }

    /**
     * @brief Batch add IP addresses to whitelist
     * @param ips List of IP addresses
     * @return Number of successfully added entries
     */
    size_t add_ips(const std::vector<std::string>& ips) {
        size_t count = 0;
        for (const auto& ip : ips) {
            if (add_ip(ip)) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Batch add CIDR networks to whitelist
     * @param cidrs List of CIDR networks
     * @return Number of successfully added entries
     */
    size_t add_cidrs(const std::vector<std::string>& cidrs) {
        size_t count = 0;
        for (const auto& cidr : cidrs) {
            if (add_cidr(cidr)) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Get detailed whitelist information
     * @return String containing all whitelist entries
     */
    std::string to_string() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::ostringstream oss;
        
        // Single IP addresses
        if (!single_ips_.empty()) {
            oss << "Single IPs (" << single_ips_.size() << "):\n";
            for (const auto& ip : single_ips_) {
                oss << "  - " << ip.to_string() << "\n";
            }
        }
        
        // IPv4 CIDR networks
        if (!cidr_v4_networks_.empty()) {
            oss << "IPv4 CIDR Networks (" << cidr_v4_networks_.size() << "):\n";
            for (const auto& network : cidr_v4_networks_) {
                oss << "  - " << network.network().to_string()
                    << "/" << network.prefix_length() << "\n";
            }
        }
        
        // IPv6 CIDR networks
        if (!cidr_v6_networks_.empty()) {
            oss << "IPv6 CIDR Networks (" << cidr_v6_networks_.size() << "):\n";
            for (const auto& network : cidr_v6_networks_) {
                oss << "  - " << network.network().to_string()
                    << "/" << network.prefix_length() << "\n";
            }
        }
        
        // IP ranges
        if (!ip_ranges_.empty()) {
            oss << "IP Ranges (" << ip_ranges_.size() << "):\n";
            for (const auto& range : ip_ranges_) {
                oss << "  - " << range.first.to_string()
                    << " - " << range.second.to_string() << "\n";
            }
        }
        
        // Regex patterns
        if (!regex_patterns_.empty()) {
            oss << "Regex Patterns (" << regex_patterns_.size() << "):\n";
            for (size_t i = 0; i < regex_patterns_.size(); ++i) {
                oss << "  - Pattern " << (i + 1) << ": (regex pattern)\n";
            }
        }
        
        if (empty()) {
            oss << "Whitelist is empty.\n";
        }
        
        return oss.str();
    }
    
    /**
     * @brief Get list of single IP addresses
     * @return String vector containing all single IP addresses
     */
    std::vector<std::string> get_single_ips() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<std::string> result;
        result.reserve(single_ips_.size());
        
        for (const auto& ip : single_ips_) {
            result.push_back(ip.to_string());
        }
        
        return result;
    }
    
    /**
     * @brief Get list of CIDR networks
     * @return String vector containing all CIDR networks
     */
    std::vector<std::string> get_cidr_networks() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<std::string> result;
        result.reserve(cidr_v4_networks_.size() + cidr_v6_networks_.size());
        
        for (const auto& network : cidr_v4_networks_) {
            result.push_back(network.network().to_string() + "/" +
                           std::to_string(network.prefix_length()));
        }
        
        for (const auto& network : cidr_v6_networks_) {
            result.push_back(network.network().to_string() + "/" +
                           std::to_string(network.prefix_length()));
        }
        
        return result;
    }
    
    /**
     * @brief Get list of IP ranges
     * @return String vector containing all IP ranges, format "start_ip - end_ip"
     */
    std::vector<std::string> get_ip_ranges() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        std::vector<std::string> result;
        result.reserve(ip_ranges_.size());
        
        for (const auto& range : ip_ranges_) {
            result.push_back(range.first.to_string() + " - " + range.second.to_string());
        }
        
        return result;
    }
    
    /**
     * @brief Get number of regex patterns
     * @return Number of regex patterns
     */
    size_t get_regex_pattern_count() const {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        return regex_patterns_.size();
    }

private:
    /**
     * @brief Check if IP is in specified range
     * @param ip IP address to check
     * @param start Range starting IP
     * @param end Range ending IP
     * @return true if in range
     */
    bool is_ip_in_range(const asio::ip::address& ip, 
                       const asio::ip::address& start, 
                       const asio::ip::address& end) const {
        if (ip.is_v4() && start.is_v4() && end.is_v4()) {
            auto ip_v4 = ip.to_v4().to_uint();
            auto start_v4 = start.to_v4().to_uint();
            auto end_v4 = end.to_v4().to_uint();
            return ip_v4 >= start_v4 && ip_v4 <= end_v4;
        } else if (ip.is_v6() && start.is_v6() && end.is_v6()) {
            auto ip_bytes = ip.to_v6().to_bytes();
            auto start_bytes = start.to_v6().to_bytes();
            auto end_bytes = end.to_v6().to_bytes();
            return std::memcmp(ip_bytes.data(), start_bytes.data(), 16) >= 0 &&
                   std::memcmp(ip_bytes.data(), end_bytes.data(), 16) <= 0;
        }
        return false;
    }

    mutable std::shared_mutex mutex_;
    std::unordered_set<asio::ip::address> single_ips_;
    std::vector<asio::ip::network_v4> cidr_v4_networks_;
    std::vector<asio::ip::network_v6> cidr_v6_networks_;
    std::vector<std::pair<asio::ip::address, asio::ip::address>> ip_ranges_;
    std::vector<std::regex> regex_patterns_;
};


} // namespace coro_io