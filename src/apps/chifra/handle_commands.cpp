/*-------------------------------------------------------------------------
 * This source code is confidential proprietary information which is
 * copyright (c) 2018, 2019 TrueBlocks, LLC (http://trueblocks.io)
 * All Rights Reserved
 *------------------------------------------------------------------------*/
#include "options.h"

//------------------------------------------------------------------------------------------------
bool COptions::handle_commands(void) {
    ENTER("handle_" + mode);
    nodeRequired();

    if (mode == "scrape") {
        if (isApiMode() && contains(tool_flags, "run")) {
            cout << "{ \"status\": \"cannot run\" }";
            LOG_ERR(
                "Use the API only to pause, restart, or quit the scraper -- to run, start in a new window with chifra "
                "scrape run.");
            return EXIT_FAILURE;
        }
        // setenv("FRESHEN_FLAG S", freshen_flags.c_str(), true);
    }

    // URLs require key/value pairs, command lines don't so we remove unneeded keys
    CStringArray removes = {"--names", "--terms", "--addrs"};
    for (auto remove : removes)
        if (remove != "--addrs" || mode != "blocks")
            tool_flags = substitute(tool_flags, remove, "");

    CStringArray scraper = {"--restart", "--pause", "--quit"};
    for (auto cmd : scraper)
        replace(tool_flags, cmd, substitute(cmd, "--", ""));

    ostringstream os;
    os << cmdMap[mode] << " " << tool_flags;
    for (auto addr : addrs)
        os << " " << addr;

    // clang-format off
    LOG_CALL(os.str());
    return system(os.str().c_str());
    // clang-format on
}

//------------------------------------------------------------------------------------------------
map<string, string> cmdMap = {{"slurp", "ethslurp"},        {"collections", "ethNames --collections"},
                              {"names", "ethNames"},        {"tags", "ethNames --tags"},
                              {"abis", "grabABI"},          {"blocks", "getBlock"},
                              {"transactions", "getTrans"}, {"receipts", "getReceipt"},
                              {"logs", "getLogs"},          {"traces", "getTrace"},
                              {"quotes", "ethQuote"},       {"state", "getState"},
                              {"tokens", "getTokenInfo"},   {"when", "whenBlock"},
                              {"where", "whereBlock"},      {"status", "cacheStatus"},
                              {"rm", "acctExport --rm"},    {"list", "acctExport --list"},
                              {"export", "acctExport"},     {"scrape", "blockScrape"},
                              {"dive", "turboDive"},        {"serve", "tbServer"},
                              {"pins", "pinStatus"},        {"explore", "ethscan.py"}};
