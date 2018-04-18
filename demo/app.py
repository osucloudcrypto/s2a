from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import flask
from flask import request

import subprocess

app = flask.Flask('dsse_demo')

CLIENT_PATH = "../src/client"
CLIENT_DIR = "../src"

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
    return flask.render_template("search.html", results=results)

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
    app.run(debug=True)
