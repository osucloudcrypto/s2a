# encoding: utf8

"""prototype of https://eprint.iacr.org/2014/853.pdf

implements the basic dynamic scheme Π^+_{bas}"""

import hmac
import hashlib
import os

class DSSE():
    def __init__(self):
        self.D = {}
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

        self.D = dict(L) # server state
        self.K = k # client state

        # page 17
        # client state
        self.Kplus = random_key()
        self.Dcount = {}
        # server state
        self.Dplus = {}

    def search_client(self, w):
        K1 = self._mac(self.K, '1' + w)
        K2 = self._mac(self.K, '2' + w)
        K1plus = self._mac(self.Kplus, '1' + w)
        K2plus = self._mac(self.Kplus, '2' + w)
        return self.search_server(K1, K2, K1plus, K2plus)

    def search_server(self, K1, K2, K1plus, K2plus):
        c = 0
        ids = []
        while True:
            l = self._mac(K1, str(c))
            d = self.D.get(l, None)
            if d is None:
                break
            id = self._dec(K2, d)
            ids.append(id)
            c += 1
        # page 18
        c = 0
        while True:
            l = self._mac(K1plus, str(c))
            d = self.Dplus.get(l, None)
            if d is None:
                break
            id = self._dec(K2plus, d)
            ids.append(id)
            c += 1
        return ids

    def update_client(self, action, id, W):
        if action == 'add' or action == 'edit':
            L = []
            for w in W:
                K1 = self._mac(self.Kplus, '1' + w)
                K2 = self._mac(self.Kplus, '2' + w)
                c = self.Dcount.get(w, 0)
                l = self._mac(K1, str(c))
                d = self._enc(K2, id)
                c += 1
                self.Dcount[w] = c
                L.append((l, d))
            L.sort()
            return self.update_server(action, L)
        else:
            raise ValueError("invalid action:", action)

    def update_server(self, action, L):
        if action == 'add' or action == 'edit':
            # page 18
            for l, d in L:
                assert l not in self.Dplus
                self.Dplus[l] = d
        else:
            raise ValueError("invalid action:", action)

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
    print("setup")
    dsse.setup(list(db.keys()), db)
    print(dsse.search_client('none'))
    print(dsse.search_client('hello'))
    print(dsse.search_client('test'))
    print(dsse.search_client('foo'))

    print("update")
    dsse.update_client('add', 'c', ['hello', 'foo'])
    print(dsse.search_client('none'))
    print(dsse.search_client('hello'))
    print(dsse.search_client('test'))
    print(dsse.search_client('foo'))

main()
