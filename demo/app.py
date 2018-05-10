from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import flask
from flask import request
import werkzeug.security

import subprocess
import os
import re


app = flask.Flask('dsse_demo')

DB_PATH = os.path.expanduser("~/IM-DSSE/IM-DSSE/data/DB")
CLIENT_PATH = "../src/client"
CLIENT_DIR = "../src"

def strip_prefix(s, prefix):
    if s.startswith(prefix):
        s = s[len(prefix):]
    return s

@app.route("/")
def index():
    return flask.render_template('index.html')

@app.route("/search")
def search():
    query = request.args.get("query", u"")
    try:
        results = run_client(query)
    except subprocess.CalledProcessError as e:
        print(e.output)
        raise e
    results = [strip_prefix(x, DB_PATH) for x in results]
    return flask.render_template("search.html", results=results, query=query)

@app.route("/view/<path:path>")
def view(path):
    highlight = flask.request.args.get("highlight", None)
    return view_file(path, highlight)

def view_file(webpath, highlight=None):
    path = werkzeug.security.safe_join(DB_PATH, webpath.strip('/'))
    with open(path) as f:
        contents = f.read()

    if highlight:
        chunks = split_word(contents, highlight)
    else:
        chunks = [contents]
    return flask.render_template("view.html", path=webpath, contents=contents, chunks=chunks, highlight=highlight)

def split_word(s, word):
    chunks = re.split("("+re.escape(word)+")", s)
    return chunks

@app.route
def update(query):
    """"""


def run_client(query):
    """perform a search for the given query.

    returns a list of filenames.
    raises subprocess.CalledProcessError on error.
    """
    output = subprocess.check_output(
        [
            CLIENT_PATH,
            "search",
            query,
        ],
        cwd=CLIENT_DIR,
    )
    results = []
    for line in output.splitlines():
        if line.startswith(query+":"):
            filename = line.split(":", 1)[1].strip()
            results.append(filename)
    return results


if __name__ == '__main__':
    app.run(debug=True, port=18090)
