from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import flask
from flask import request
import werkzeug.security

import errno
import subprocess
import os
import re


app = flask.Flask('dsse_demo')

#DB_PATH = os.path.expanduser("~/IM-DSSE/IM-DSSE/data/DB")
DB_PATH = os.path.expanduser("~/maildir")
UPDATE_PATH = "./updates"
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
    return view_or_edit_file(path, "view.html", highlight=highlight)

@app.route("/edit/<path:path>", methods=["GET"])
def edit_view(path):
    return view_or_edit_file(path, "edit.html", highlight="")

def split_word(s, word):
    chunks = re.split("("+re.escape(word)+")", s)
    return chunks

def view_or_edit_file(original_path, template, highlight):
    webpath = original_path.strip('/')

    # try to look up the file in the update ptah first
    path = werkzeug.security.safe_join(UPDATE_PATH, webpath)
    if not os.path.exists(path):
        # otherwise fall back on the db path
        path = werkzeug.security.safe_join(DB_PATH, webpath)

    try:
        with open(path) as f:
            contents = f.read()
    except IOError:
        flask.abort(404)

    if highlight:
        chunks = split_word(contents, highlight)
    else:
        chunks = [contents]
    return flask.render_template(template, path=original_path, contents=contents, chunks=chunks, highlight=highlight)

@app.route("/update", methods=["POST"])
def update():
    webpath = flask.request.form['path']
    newcontents = flask.request.form['contents']

    oldpath = werkzeug.security.safe_join(DB_PATH, webpath)

    # if file has not been edited
    # -> need to add all new words and delete all old words

    # if file has been edited before
    # -> need to add all words which are in the new contents but not in the current contents
    # -> need to delete all words which are in the new contents but not in the current contents

    newpath = werkzeug.security.safe_join(UPDATE_PATH, webpath)
    if not os.path.exists(newpath):
        oldpath = werkzeug.security.safe_join(DB_PATH, webpath)
    else:
        oldpath = newpath

    try:
        with open(oldpath) as f:
            oldcontents = ""
    except IOError:
        oldcontents = ""

    added, deleted = worddifference(oldcontents, newcontents)

    # run client add 42 added...
    # run client delete 42 deleted...

    try:
        os.makedirs(os.path.dirname(newpath))
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise

    with open(newpath, "w") as f:
        f.write(newcontents)

    return flask.redirect(flask.url_for("view", path=webpath))

def worddifference(a, b):
    """returns the set of added and delete words to go from string a to string b"""

    a = set(a.split())
    b = set(b.split())

    added = b - a
    deleted = a - b

    added = sorted(added)
    deleted = sorted(deleted)

    return added, deleted

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
