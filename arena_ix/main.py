#!/usr/bin/env python
# -*- coding: utf-8 -*-

import random, time, sqlite3, requests
import castleage
from datetime import datetime


conn = sqlite3.connect('castleage.sqlite')
players = [ castleage.Player(cfg[0], cfg[1], cfg[2], cfg[3]) for cfg in conn.execute('SELECT email, password, duel_higher, duel_lower FROM config') ]
conn.close()

while True:

    try:
        print datetime.now()

        for player in players:
            try:
                player.arena_duel()
            except requests.exceptions.ConnectionError as e:
                if player is not None and player.name is not None:
                    print player.name, e
                else:
                    print e

        sleep_time = random.randint(180, 250)
        print 'Sleep %d seconds...\n' % sleep_time
        time.sleep(sleep_time)

    except KeyboardInterrupt:
        print 'Bye Bye!!!'
        break
