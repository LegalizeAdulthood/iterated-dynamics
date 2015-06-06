var fs = require('fs');
var split = require('split');
var async = require('async');

function processCommand(state, cmd) {
    var matches;
    var command;
    var arg;
    if (matches = cmd.match(/^([^=]+)=(.+)$/)) {
        command = matches[1];
        arg = matches[2];
    } else if (matches = cmd.match(/^Format([+-]*)$/)) {
        state.formatting = matches[1] === '+';
        return;
    } else if (cmd.match(/^DocContents$/)) {
        state.content = 'toc';
        state.tableOfContents = [];
        return;
    } else if (matches = cmd.match(/^Doc([+-])$/)) {
        state.doc = matches[1] === '+';
        return;
    } else if (matches = cmd.match(/^Online([+-])$/)) {
        state.online = matches[1] === '+';
        return;
    } else if (cmd.match(/^FF$/)) {
        return;
    } else if (cmd.match(/^OnlineFF$/i)) {
        return;
    } else if (matches = cmd.match(/^Include (.+)$/)) {
        return;
    } else if (matches = cmd.match(/^CompressSpaces([+-])/)) {
        state.compressSpaces = matches[1] === '+';
        return;
    } else if (cmd.match(/^EndTable$/)) {
        delete state.table;
        return;
    } else if (matches = cmd.match(/^FormatExclude([+-])$/)) {
        state.formatExcludeEnabled = matches[1] === '+';
        return;
    } else {
        throw 'Unknown command with no argument: "' + cmd + '"';
    }
    switch (command.toLowerCase()) {
    case 'hdrfile':
        state.headerFile = arg;
        break;

    case 'hlpfile':
        state.helpFile = arg;
        break;

    case 'version':
        state.version = arg;
        break;

    case 'formatexclude':
        if (state.topic.length === 0) {
            state.globalFormatExclude = Number(arg);
        } else {
            state.formatExclude = Number(arg);
        }
        break;

    case 'label':
        if (state.topic) {
            state.labels[arg] = state.topic;
        } else {
            throw 'Label ' + arg + ' has no topic!';
        }
        break;

    case 'topic':
        state.content = 'topic';
        state.topic = arg;
        if (matches = arg.match(/^(.+), Label=(.+)$/)) {
            state.topic = matches[1];
            state.labels[matches[2]] = state.topic;
        }
        state.topics[state.topic] = [];
        state.topicNames.push(state.topic);
        break;

    case 'table':
        state.content = 'table';
        state.table = arg;
        state.tables[state.table] = [];
        break;

    case 'data':
        state.content = 'data';
        state.data = arg;
        state.datas[state.data] = [];
        break;

    default:
        throw 'Unknown command ' + command + ': "' + cmd + '"';
    }
    ++state.commandCount;
}

function processLine(state, line) {
    if (line.match(/^;/)) {
        return;
    } else if (line.match(/^~/)) {
        if (line.match(/,/) && !line.match(/\\,/)) {
            line.substr(1).split(/, */).forEach(function(cmd) {
                processCommand(state, cmd);
            });
        } else {
            processCommand(state, line.substr(1));
        }
    } else {
        // content line
        switch (state.content) {
        case 'toc':
            state.tableOfContents.push(line);
            break;
        case 'topic':
            ++state.contentCount;
            if (state.topic.length) {
                state.topics[state.topic].push(line);
            } else {
                throw 'Content line "' + line + '" has no topic!';
            }
            break;
        case 'table':
            state.tables[state.table].push(line);
            break;
        case 'data':
            state.datas[state.data].push(line);
            break;
        default:
            throw 'Unknown content state: ' + state.content;
        }
    }
}

function writeRst(state) {
    var i, fileName;
    for (i = 0; i < state.topicNames.length; ++i) {
        fileName = state.topicNames[i].toLowerCase().replace(/[^0-9a-z-]+/g, '_') + '.rst';
        console.log('Topic: ' + state.topicNames[i] + ', file: ' + fileName);
    }
    console.log(state.contentCount + ' lines, '
        + state.commandCount + ' commands, '
        + Object.keys(state.topics).length + ' topics, '
        + Object.keys(state.labels).length + ' labels, '
        + Object.keys(state.datas).length + ' datas, '
        + Object.keys(state.tables).length + ' tables.');
}

function parseFile(state, file, next) {
    state.files.push(file);
    state.file = file;
    fs.createReadStream(file)
        .pipe(split())
        .on('data', function(line) {
            processLine(state, line);
        })
        .on('end', function() {
            next(null);
        });
}

var state = {
    files: [],
    topics: {},
    topicNames: [],
    datas: {},
    tables: {},
    labels: {},
    topic: '',
    commandCount: 0,
    contentCount: 0
};
async.forEachSeries(['help.src', 'help2.src', 'help3.src', 'help4.src', 'help5.src'],
    function(file, next) {
        parseFile(state, '../hc/' + file, next);
    },
    function(err) {
        if (err) {
            throw err;
        }
        writeRst(state);
    });
