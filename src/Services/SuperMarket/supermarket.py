#!/usr/bin/env python3
"""supermarket — command line access to the shared list served by go.php.

Install, from this directory:
    ln -s "$PWD/supermarket.py" ~/.local/bin/supermarket

Configure in ~/.bashrc:
    export SUPERMARKET_ID="idhere"
    export SUPERMARKET_SITE="http://ammar.gr/supermarket/"

    supermarket list [-a]
    supermarket add "Product Description" [N]
    supermarket remove "Product Description"
"""

import json
import os
import sys
import unicodedata
import urllib.error
import urllib.parse
import urllib.request
import uuid

# digraphs first: each rule outputs Latin, so no later rule can re-match it
GREEKLISH = [
    ('ου', 'u'), ('ει', 'i'), ('οι', 'i'), ('υι', 'i'), ('αι', 'e'),
    ('α', 'a'), ('β', 'v'), ('γ', 'g'), ('δ', 'd'), ('ε', 'e'), ('ζ', 'z'),
    ('η', 'i'), ('θ', 'th'), ('ι', 'i'), ('κ', 'k'), ('λ', 'l'), ('μ', 'm'),
    ('ν', 'n'), ('ξ', 'ks'), ('ο', 'o'), ('π', 'p'), ('ρ', 'r'), ('σ', 's'),
    ('τ', 't'), ('υ', 'i'), ('φ', 'f'), ('χ', 'x'), ('ψ', 'ps'), ('ω', 'o'),
]

MAX_NAME_LEN = 100      # kept in step with configuration.default.php
MAX_QTY = 999
DEFAULT_TITLE = 'Λίστα Σούπερ Μάρκετ'
TIMEOUT = 10

USAGE = """usage: supermarket list [-a]
       supermarket add "Product Description" [N]
       supermarket remove "Product Description"

  list    print what is still to buy; -a also lists what is in the basket
  add     put the product on the list; N adds that many to what is already there
  remove  delete the product, same as the trash button in the browser

Reads SUPERMARKET_ID and SUPERMARKET_SITE from the environment."""


def die(msg, code=1):
    print(msg, file=sys.stderr)
    sys.exit(code)


def endpoint():
    """Base URL of this user's cart, or exit 2 explaining what is missing."""
    site = os.environ.get('SUPERMARKET_SITE', '').strip()
    token = os.environ.get('SUPERMARKET_ID', '').strip()
    if not site:
        die('SUPERMARKET_SITE is not set. Add to ~/.bashrc:\n'
            '    export SUPERMARKET_SITE="http://ammar.gr/supermarket/"', 2)
    if not token:
        die('SUPERMARKET_ID is not set. Add to ~/.bashrc:\n'
            '    export SUPERMARKET_ID="idhere"', 2)
    # go.php directly: index.php only redirects there.
    return site.rstrip('/') + '/go.php?i=' + urllib.parse.quote(token)


def encode_multipart(fields):
    """multipart/form-data body : the AmmarServer backend's POST parser only
       understands multipart ( it has no application/x-www-form-urlencoded
       fallback ), same wire format its own web UI already sends via FormData."""
    boundary = uuid.uuid4().hex
    parts = []
    for name, value in fields.items():
        parts.append(
            '--%s\r\nContent-Disposition: form-data; name="%s"\r\n\r\n%s\r\n'
            % (boundary, name, value))
    parts.append('--%s--\r\n' % boundary)
    body = ''.join(parts).encode('utf-8')
    return body, 'multipart/form-data; boundary=%s' % boundary


def request(url, fields=None):
    """GET the cart, or POST a mutation. Either way the server replies with the cart."""
    req = url
    if fields:
        data, content_type = encode_multipart(fields)
        req = urllib.request.Request(url, data=data, headers={'Content-Type': content_type})
    try:
        with urllib.request.urlopen(req, timeout=TIMEOUT) as r:
            body = r.read().decode('utf-8')
    except urllib.error.HTTPError as e:
        die('Server returned HTTP %s' % e.code)
    except urllib.error.URLError as e:
        die('Cannot reach %s (%s)' % (url.split('?')[0], e.reason))
    try:
        cart = json.loads(body)
    except ValueError:
        die('Server did not return JSON — is SUPERMARKET_SITE right?')
    if not isinstance(cart, dict) or not isinstance(cart.get('items'), list):
        die('Unexpected reply from the server')
    return cart


def exact(items, name):
    """Items whose name sounds like `name`."""
    key = norm(name)
    return [it for it in items if norm(it.get('n', '')) == key]


def find(items, name):
    """Exact matches if there are any, otherwise substring matches."""
    key = norm(name)
    return exact(items, name) or [it for it in items if key in norm(it.get('n', ''))]


def norm(s):
    """Greeklish key, kept in step with norm() in go.php: names are compared by
       how they sound, so "Ψωμι", the misspelt "Ψωμή" and a typed "psomi" all
       reduce to "psomi" and find the same ψωμί. casefold() folds case and final
       sigma; NFD-minus-combining takes the accents and διαλυτικά off. The
       digraphs are substituted before the single letters they contain, and
       every replacement lands on Latin, which no later rule can match."""
    s = unicodedata.normalize('NFD', s.strip().casefold())
    s = ''.join(c for c in s if not unicodedata.combining(c))
    for greek, latin in GREEKLISH:
        s = s.replace(greek, latin)
    return s


def label(it):
    q = it.get('q', 1)
    return it.get('n', '') + ('  ×%d' % q if q > 1 else '')


def cmd_list(url, show_done):
    cart = request(url + '&api=1')
    items = cart['items']
    todo = [it for it in items if not it.get('c')]
    done = [it for it in items if it.get('c')]
    fancy = sys.stdout.isatty()

    if not items:
        print('The list is empty.' if fancy else '')
        return
    if fancy:
        print('🛒 %s\n' % ((cart.get('name') or '').strip() or DEFAULT_TITLE))
    for it in todo:
        print('  %s' % label(it) if fancy else label(it))
    if not todo and fancy:
        print('  Nothing left to buy ✨')
    # bought items stay folded away unless asked for, as in the web UI, where a
    # fresh list arrives with all 52 seed staples already ticked off
    if done and show_done:
        if fancy:
            print('\n  ── in the basket ──')
        for it in done:
            print('  ✓ %s' % label(it) if fancy else '[x] %s' % label(it))
    if fancy:
        hint = '' if (show_done or not done) else '  (supermarket list -a)'
        print('\n%d to buy, %d in the basket%s' % (len(todo), len(done), hint))


def cmd_add(url, name, count):
    name = name.strip()
    if not name:
        die('Empty product name', 2)
    if len(name) > MAX_NAME_LEN:
        # the server drops over-long names but still answers 200, so catch it here
        die('Name is %d characters, the server accepts at most %d'
            % (len(name), MAX_NAME_LEN), 2)

    # what the item was at before, so an explicit count can be added onto it
    was = exact(request(url + '&api=1')['items'], name)

    # creates the item at q=1, or just unticks it and leaves q alone
    cart = request(url, {'a': 'add', 'n': name})
    match = exact(cart['items'], name)
    if not match:
        die('The server did not accept «%s»' % name)
    item = match[0]

    if count is not None:
        target = (was[0].get('q', 1) if was else 0) + count
        if target > MAX_QTY:
            print('Capped at %d, the server maximum' % MAX_QTY, file=sys.stderr)
            target = MAX_QTY
        # absolute v=, not d=: the add above already put a new item at 1
        if item.get('q') != target:
            cart = request(url, {'a': 'qty', 'id': item['id'], 'v': target})
            item = next((it for it in cart['items'] if it['id'] == item['id']), item)

    print('+ %s' % label(item))


def cmd_remove(url, name):
    name = name.strip()
    if not name:
        die('Empty product name', 2)
    hits = find(request(url + '&api=1')['items'], name)
    if not hits:
        die('Not on the list: «%s»' % name)
    if len(hits) > 1:
        print('«%s» matches %d products, nothing removed:' % (name, len(hits)),
              file=sys.stderr)
        for it in hits:
            print('  %s' % it.get('n', ''), file=sys.stderr)
        sys.exit(1)
    request(url, {'a': 'del', 'id': hits[0]['id']})
    print('- %s' % hits[0].get('n', ''))


def main():
    args = sys.argv[1:]
    if args and args[0] in ('-h', '--help', 'help'):
        print(USAGE)
        return
    if not args:
        die(USAGE, 2)

    cmd, rest = args[0], args[1:]
    url = endpoint()

    if cmd == 'list' and not [a for a in rest if a not in ('-a', '--all')]:
        cmd_list(url, show_done=bool(rest))
    elif cmd == 'add' and 1 <= len(rest) <= 2:
        count = None
        if len(rest) == 2:
            if not rest[1].isdigit() or int(rest[1]) < 1:
                die('Quantity must be a positive whole number, got «%s»' % rest[1], 2)
            count = int(rest[1])
        cmd_add(url, rest[0], count)
    elif cmd == 'remove' and len(rest) == 1:
        cmd_remove(url, rest[0])
    elif cmd in ('list', 'add', 'remove'):
        die('Bad arguments for «%s»\n\n%s' % (cmd, USAGE), 2)
    else:
        die('Unknown command «%s»\n\n%s' % (cmd, USAGE), 2)


if __name__ == '__main__':
    main()
