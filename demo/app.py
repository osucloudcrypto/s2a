import flask

app = flask.Flask('dsse_demo')

@app.route("/")
def hello():
    return u'Hello world'


if __name__ == '__main__':
    app.run(debug=True)
