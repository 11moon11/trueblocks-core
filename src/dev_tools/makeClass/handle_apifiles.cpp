/*-------------------------------------------------------------------------------------------
 * qblocks - fast, easily-accessible, fully-decentralized data from blockchains
 * copyright (c) 2018, 2019 TrueBlocks, LLC (http://trueblocks.io)
 *
 * This program is free software: you may redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version. This program is
 * distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details. You should have received a copy of the GNU General
 * Public License along with this program. If not, see http://www.gnu.org/licenses/.
 *-------------------------------------------------------------------------------------------*/
#include "options.h"

extern const char* STR_PATH_ENTRY;
extern const char* STR_PATH_PARAM;
//---------------------------------------------------------------------------------------------------
void COptions::writeOpenApiFile(void) {
    if (!openapi)
        return;
    LOG_INFO(cYellow, "handling openapi file...", cOff);
    counter = CCounter();  // reset

    CCommands commands;
    options_2_Commands(commands);

    CStringArray endpoints{
        "Accounts,export", "Accounts,list", "Accounts,tags", "Accounts,names", "Accounts,collections", "Accounts,abis",
        "Accounts,rm",     "Admin,status",  "Admin,scrape",  "Data,blocks",    "Data,transactions",    "Data,receipts",
        "Data,logs",       "Data,traces",   "Data,when",     "State,state",    "State,tokens",         "Other,quotes",
        "Other,slurp",     "Other,where",   "Other,dive",
    };

    ostringstream pathStream;
    for (auto ep : endpoints) {
        string_q entry = STR_PATH_ENTRY;
        CStringArray parts;
        explode(parts, ep, ',');
        replace(entry, "[{TAGS}]", parts[0]);
        replace(entry, "[{PATH}]", parts[1]);
        CCommandOptionArray params;
        CCommandOptionArray notes;
        CCommandOptionArray errors;
        CCommandOptionArray descr;
        select_commands(parts[1], params, notes, errors, descr);
        if (descr.size()) {
            replace(entry, "[{SUMMARY}]", descr[0].description);
            replace(entry, "[{DESCR}]", descr[0].description);
        } else {
            replace(entry, "[{SUMMARY}]", "");
            replace(entry, "[{DESCR}]", "");
        }
        replace(entry, "[{ID}]", toLower(parts[0] + "-" + parts[1]));
        ostringstream paramStream;
        for (auto param : params) {
            string_q p = STR_PATH_PARAM;
            replace(p, "[{NAME}]", param.command);
            replace(p, "[{IN}]", (param.option_kind == "positional" ? "query" : "query"));
            replace(p, "[{DESCR}]", param.description);
            replace(p, "[{REQ}]", param.is_required == "true" ? "true" : "false");
            if (contains(param.data_type, "boolean")) {
                const char* BOOL_STR =
                    "type: string\n"
                    "          enum:\n"
                    "          - \"\"\n"
                    "          - \"true\"";
                replace(p, "type: string", BOOL_STR);
            } else {
                if (!contains(param.data_type, "list") &&
                    (contains(param.data_type, "uint") || contains(param.data_type, "double")))
                    replace(p, "type: string", "type: number");
            }
            counter.nVisited++;
            paramStream << p << endl;
        }
        replace(entry, "[{PARAMS}]", paramStream.str());
        pathStream << entry;
        counter.nProcessed++;
    }

    string_q sourceFn = "../../trueblocks-explorer/yaml-resolved/swagger.yaml";
    string_q templateFn = "../src/dev_tools/makeClass/templates/swagger.yaml";
    if (!test && fileExists(sourceFn) && fileExists(templateFn)) {
        string_q orig = asciiFileToString(sourceFn);
        string_q str = asciiFileToString(templateFn);
        string_q ps = pathStream.str();
        replaceReverse(ps, "\n", "");
        replace(str, "[{PATHS}]", ps);
        if (orig != str)
            stringToAsciiFile(sourceFn, str);
    }
    if (test) {
        counter.nProcessed = 0;
        LOG_WARN("Testing only - openapi file not written");
    }
    LOG_INFO(cYellow, "makeClass --options --openapi", cOff, " processed ", counter.nProcessed, " routes and ",
             counter.nVisited, " commands.", string_q(40, ' '));
}

//---------------------------------------------------------------------------------------------------
void COptions::writeApiFile(void) {
    if (!api)
        return;

    LOG_INFO(cYellow, "handling api file...", cOff);
    counter = CCounter();  // reset

    CCommands commands;
    options_2_Commands(commands);

    ostringstream routeStream;
    bool firstRoute = true;
    routeStream << "{" << endl;
    for (auto route : commands.routes) {
        counter.nProcessed++;
        if (!firstRoute)
            routeStream << "," << endl;
        routeStream << "  \"" << route.route << "\": {" << endl;
        bool firstCmd = true;
        for (auto command : route.commands) {
            if (command.option_kind != "note") {
                counter.nVisited++;
                if (command.hotkey.empty())
                    command.hotkey = "<not-set>";
                if (command.command.empty())
                    command.command = "<not-set>";
                if (command.data_type.empty())
                    command.data_type = "<not-set>";

                ostringstream commandStream;
                if (!firstCmd)
                    commandStream << "," << endl;
                commandStream << "    ";
                commandStream << "\"" << command.command << "\": " << command;

                string_q str = commandStream.str();
                replaceAll(str, "\"true\"", "true");
                replaceAll(str, "\"false\"", "false");
                replaceAll(str, "NOPOS", "");
                replaceAll(str, "\"def_val\": false,", "\"def_val\": \"\",");
                replaceAll(str, "\"def_val\": true,", "\"def_val\": \"true\",");
                replaceAll(str, "\"def_val\": \"\"\"\",", "\"def_val\": \"\",");
                replaceAll(str, "<not-set>", "");
                replaceReverse(str, "\n", "");
                routeStream << str;
                firstCmd = false;
            }
        }
        routeStream << endl << "  }";
        firstRoute = false;
    }
    routeStream << endl << "}" << endl;
    if (!test && folderExists("../../trueblocks-explorer/api/"))
        stringToAsciiFile("../../trueblocks-explorer/api/api_options.json", routeStream.str());
    if (test) {
        counter.nProcessed = 0;
        LOG_WARN("Testing only - api file not written");
    }
    LOG_INFO(cYellow, "makeClass --options --api", cOff, " processed ", counter.nProcessed, " routes and ",
             counter.nVisited, " commands.", string_q(40, ' '));
}

//---------------------------------------------------------------------------------------------------
void COptions::options_2_Commands(CCommands& commands) {
    CApiRoute curRoute;
    for (auto option : optionArray) {
        if (curRoute.route != option.api_route) {
            if (!curRoute.route.empty())
                commands.routes.push_back(curRoute);
            curRoute = CApiRoute();
            curRoute.route = option.api_route;
        }
        curRoute.commands.push_back(option);
    }
    commands.routes.push_back(curRoute);
    expContext().lev = 2;
    HIDE_FIELD(CCommandOption, "is_customizable");
}

//---------------------------------------------------------------------------------------------------
void COptions::select_commands(const string_q& cmd, CCommandOptionArray& cmds, CCommandOptionArray& notes,
                               CCommandOptionArray& errors, CCommandOptionArray& descr) {
    for (auto option : optionArray) {
        if (option.api_route == cmd) {
            if (option.option_kind == "description") {
                descr.push_back(option);
            } else if (option.option_kind == "note") {
                notes.push_back(option);
            } else if (option.option_kind == "error") {
                errors.push_back(option);
            } else if (option.option_kind != "deprecated") {
                cmds.push_back(option);
            }
        }
    }
}

//---------------------------------------------------------------------------------------------------
const char* STR_PATH_ENTRY =
    "  /[{PATH}]:\n"
    "    get:\n"
    "      tags:\n"
    "      - [{TAGS}]\n"
    "      summary: \"[{SUMMARY}]\"\n"
    "      description: \"[{DESCR}]\"\n"
    "      operationId: \"[{ID}]\"\n"
    "      parameters:\n"
    "[{PARAMS}]"
    "      responses:\n"
    "        \"200\":\n"
    "          description: status of the scraper\n"
    "          content:\n"
    "            application/json:\n"
    "              schema:\n"
    "                type: array\n"
    "                items:\n"
    "                  $ref: '#/components/schemas/response'\n"
    "        \"400\":\n"
    "          description: bad input parameter\n";

const char* STR_PATH_PARAM =
    "      - name: [{NAME}]\n"
    "        in: [{IN}]\n"
    "        description: [{DESCR}]\n"
    "        required: [{REQ}]\n"
    "        style: form\n"
    "        explode: true\n"
    "        schema:\n"
    "          type: string";
