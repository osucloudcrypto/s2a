# encoding: utf8

"""prototype of https://eprint.iacr.org/2014/853.pdf

implements the basic dynamic scheme Π^+_{bas}"""

import hmac
import hashlib
import os

class DSSE():
    def __init__(self):
        self.map = {}
        self.K = None # master key

    def setup(self, tokens, db):
        """Initialize the encrypted index

        tokens - list of [keyword]
        db - dict of {keyword: [fileid]}
        """
        # Figure 5 (page 8)
        k = random_key()
        L = []
        for w in tokens:
            K1 = self._mac(k, '1' + w)
            K2 = self._mac(k, '2' + w)
            c = 0
            for id in db[w]:
                l = self._mac(K1, str(c))
                d = self._enc(K2, id)
                L.append((l, d))
                c += 1

        L.sort()
        self.map = dict(L)
        self.K = k

    def search_client(self, w):
        K1 = self._mac(self.K, '1' + w)
        K2 = self._mac(self.K, '2' + w)
        # send K1,K2 to server
        return self.search_server(K1, K2)

    def search_server(self, K1, K2):
        c = 0
        ids = []
        while True:
            t = self._mac(K1, str(c))
            d = self.map.get(t, None)
            if d is None:
                break
            id = self._dec(K2, d)
            ids.append(id)
            c += 1
        return ids

    @staticmethod
    def _mac(k, s):
        h = hmac.new(k, s, hashlib.sha256)
        return h.digest()

    @staticmethod
    def _enc(k, m):
        return m # XXX

    @staticmethod
    def _dec(k, m):
        return m # XXX

def random_key():
    return os.urandom(256//8)

def main():
    db = {
        'hello': ['a', 'b'],
        'test': ['a'],
    }
    dsse = DSSE()
    dsse.setup(list(db.keys()), db)
    print(dsse.search_client('none'))
    print(dsse.search_client('hello'))
    print(dsse.search_client('test'))

main()
