var fs = require('fs');
var split = require('split');
var async = require('async');

function processCommand(state, cmd) {
console.log('command: ' + cmd);
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
        state.tableOfContents = true;
        return;
    } else if (cmd.match(/^OnlineFF$/)) {
        state.onlineFF = cmd;
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
        state.labels[arg] = '??';
        break;

    case 'topic':
        state.tableOfContents = false;
        state.topic = arg;
        if (matches = arg.match(/^(.+), Label=(.+)$/)) {
            state.topic = matches[1];
            state.labels[matches[2]] = state.topic;
        }
        state.topics[state.topic] = [];
        break;

    case 'table':
        state.table = arg;
        break;

    case 'data':
        state.data = arg;
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
        ++state.contentCount;
        if (state.topic.length) {
            state.topics[state.topic].push(line);
        } else {
            throw 'Content line "' + line + '" has no topic!';
        }
    }
}

function translateFile(file, next) {
    var state = {
        topics: {},
        labels: {},
        topic: '',
        commandCount: 0,
        contentCount: 0
    };
    fs.createReadStream(file)
        .pipe(split())
        .on('data', function(line) {
            processLine(state, line);
        })
        .on('end', function() {
            console.log('File ' + file + ': '
                + state.contentCount + ' lines, '
                + state.commandCount + ' commands, '
                + Object.keys(state.topics).length + ' topics, '
                + Object.keys(state.labels).length + ' labels.');
            next(null);
        });
}

var files = [
    '../hc/help.src',
    '../hc/help2.src',
    '../hc/help3.src',
    '../hc/help4.src',
    '../hc/help5.src'
];
async.eachSeries(files, translateFile, function(err) {
    if (err) {
        throw err;
    }
});
