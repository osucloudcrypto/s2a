# encoding: utf8

"""prototype of https://eprint.iacr.org/2014/853.pdf

implements the basic dynamic scheme Π^{dyn_{bas}"""

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

        # page 19
        self.Kminus = random_key()
        self.Srev = set()

    def search_client(self, w):
        # page  8
        K1 = self._mac(self.K, '1' + w)
        K2 = self._mac(self.K, '2' + w)
        # page 18
        K1plus = self._mac(self.Kplus, '1' + w)
        K2plus = self._mac(self.Kplus, '2' + w)
        # page 20
        K1minus = self._mac(self.Kminus, '1' + w)
        return self.search_server(K1, K2, K1plus, K2plus, K1minus)

    def search_server(self, K1, K2, K1plus, K2plus, K1minus):
        c = 0
        ids = []
        while True:
            l = self._mac(K1, str(c))
            d = self.D.get(l, None)
            if d is None:
                break
            id = self._dec(K2, d)
            revid = self._mac(K1minus, id) # p20
            if revid not in self.Srev: # p20
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
            revid = self._mac(K1minus, id) # p20
            if revid not in self.Srev: # p20
                ids.append(id)
            c += 1
        return ids

    def update_client(self, action, id, W):
        if action == 'add' or action == 'edit+':
            # page 17
            L = []
            W_in_order_of_Lrev = [] # p20
            for w in W:
                K1plus = self._mac(self.Kplus, '1' + w)
                K2plus = self._mac(self.Kplus, '2' + w)
                K1minus = self._mac(self.Kminus, '1' + w) # p20
                c = self.Dcount.get(w, 0)
                l = self._mac(K1plus, str(c))
                d = self._enc(K2plus, id)
                #c += 1
                #self.Dcount[w] = c
                #L.append((l, d))

                # page 20
                revid = self._mac(K1minus, id)
                L.append((l, d, revid))
                W_in_order_of_Lrev.append(w)
            L.sort()
            W_in_order_of_Lrev.sort() # UHHHH p20
            r = self.update_server(action, L)

            # page 20
            for ri, w in zip(r, W_in_order_of_Lrev):
                if not ri:
                    c = self.Dcount.get(w, 0)
                    c += 1
                    self.Dcount[w] = c
        elif action == 'del' or action == 'edit-':
            # page 20
            Lrev = []
            for w in W:
                K1minus = self._mac(self.Kminus, '1' + w)
                revid = self._mac(K1minus, id)
                Lrev.append(revid)
            Lrev.sort()
            self.update_server(action, Lrev)
        else:
            raise ValueError("invalid action:", action)

    def update_server(self, action, L):
        if action == 'add' or action == 'edit+':
            # page 18
            r = []
            for i, (l, d, revid) in enumerate(L):
                if revid in self.Srev:
                    # page 20
                    r.append(1)
                    self.Srev.remove(revid)
                else:
                    assert l not in self.Dplus
                    self.Dplus[l] = d
                    r.append(0) # page 20
            return r
        elif action == 'del' or action == 'edit-':
            # page 20
            for revid in L:
                self.Srev.add(revid)
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

    print("add")
    dsse.update_client('add', 'c', ['hello', 'foo'])
    print(dsse.search_client('none'))
    print(dsse.search_client('hello'))
    print(dsse.search_client('test'))
    print(dsse.search_client('foo'))

    print("del")
    dsse.update_client('del', 'a', ['hello', 'test'])
    print(dsse.search_client('none'))
    print(dsse.search_client('hello'))
    print(dsse.search_client('test'))
    print(dsse.search_client('foo'))

    print("del")
    dsse.update_client('del', 'c', ['hello', 'foo'])
    print(dsse.search_client('none'))
    print(dsse.search_client('hello'))
    print(dsse.search_client('test'))
    print(dsse.search_client('foo'))

main()
