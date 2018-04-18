from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import flask
from flask import request

app = flask.Flask('dsse_demo')

@app.route("/")
def index():
    return flask.render_template('index.html')

@app.route("/search")
def search():
    query = request.args.get("query", u"")
    return query

if __name__ == '__main__':
    app.run(debug=True)
