#!/usr/bin/env python
# -*- coding: utf-8 -*-

import requests, sqlite3, time, random
from lxml import etree
from lxml.cssselect import CSSSelector


AGENT = u'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.99 Safari/537.36'
URL_BASE = u'https://web3.castleagegame.com/castle_ws/'
URL_ARENA = URL_BASE + u'arena.php'
URL_HOME = URL_BASE + u'index.php'
URL_KEEP = URL_BASE + u'keep.php'
URL_LOGIN = URL_BASE + u'connect_login.php'

# pre-build css selectors
selector_global_container = CSSSelector('body center div#globalContainer')
selector_user_level = CSSSelector('div#main_sts_container div[title^="Your XP:"] a[href="keep.php"] > div')
selector_user_name = CSSSelector('div#main_bntp a[href="keep.php"]')

selector_keep_page_fbid = CSSSelector('div#app_body table.layout a[href^="keep.php?user="]')
selector_keep_page_name = CSSSelector('div#app_body table.layout div[title]')

selector_arena_page_duel_buttons = CSSSelector('''div#arena_mid div#battle_person form[onsubmit*="'arena.php'"]''')
selector_arena_page_current_token = CSSSelector('div#app_body table.layout span#guild_token_current_value')
selector_arena_page_user_stat_ref = CSSSelector('div#app_body table.layout img[src$="arena_battlepoints.gif"]')
selector_arena_page_duel_result = CSSSelector('div.results > div.result > div.result_body > div > img[src]')
selector_arena_page_duel_again_buttons = CSSSelector('''div.results > div.result > div.result_body form[onsubmit*="'arena.php'"]''')

arena_rank_class_mapping = [ '', 'Brawler', 'Swordsman', 'Warrior', 'Gladiator', 'Hero', 'Legend', 'Alpha Vanguard', 'Eternal Vanguard' ]


class Player:

    def __init__(self, email, password, arena_level_plus, arena_level_minus):

        # ca user stat
        self.level = None
        self.name = None
        self.fbid = None

        # ca arena stat
        self.arena_level = None
        self.arena_current_token = None
        self.arena_battle_point = None

        # ca account certification
        self.email = email
        self.password = password

        # arena duel parameter
        self.target_arena_level_plus = arena_level_plus
        self.target_arena_level_minus = arena_level_minus

        # target
        self.enemies = list()
        self.target = None

        # requests stuff
        self.session = MySession()
        self.session.headers.update({'User-Agent': AGENT})
        self.response = None

        # debug stuff
        self.save_arena_enemy_duel_data = False
        self.verbose = False

    def arena_duel(self):

        # try retrieving home page to check if we've logined, and login if necessary...
        self.response = self.session.get(URL_HOME)
        if self.is_login_required() and not self.execute_login():
            print 'Login is required, but we have failed to login, check login procedure!'
            return

        # now, we've logined, update user stat if necessary...
        if not self.update_user_stat_if_necessary():
            print 'User stat is missing, can not do anything...'
            return

        # retrieve arena page
        self.response = self.session.get(URL_ARENA)
        if not self.load_arena_data():
            print 'Fail to load arena data, can not duel anyone...'
            return

        if self.current_token >= 7:
            arena_duel_victory = False
            while self.current_token > 0:
                if not arena_duel_victory or self.target is None: # defeat or init
                    self.target = self.choose_arena_enemy()
                self.response = self.session.post(URL_ARENA, data=self.target.duel_data)
                arena_duel_victory = self.check_arena_duel_result() # parse response: 1. victory or defeat, 2. current token, arena level, updated duel data in target (used when victory), update ArenaEnemy list (used when defeat)
                if self.target is None:
                    print '[%s lv%d rank%d] V.S. [ previous one ] ===> KO I guess' % (self.name, self.level, self.arena_level)
                    break # some strange thing will happen after this happens, so break it now, do duel later...
                else:
                    print '[%s lv%d rank%d] V.S. [%s lv%d rank%d] ===> %s' % (self.name, self.level, self.arena_level, self.target.name, self.target.level, self.target.arena_level, ('Defeat', 'Victory')[arena_duel_victory])

                time.sleep(float(random.randint(3000, 6000))/1000.0)


    def is_login_required(self):

        login_required = not self.is_response_page_logined()

        if self.verbose:
            print 'Check if login is required for %s: %s' % (self.email, login_required)

        return login_required

    def execute_login(self):

        if self.verbose:
            print 'Execute login for %s' % self.email

        login_data = {
            "platform_action": "CA_web3_login",
            "player_email": self.email,
            "player_password": self.password,
        }

        self.response = self.session.post(URL_LOGIN, data=login_data)
        return self.is_response_page_logined()

    def is_response_page_logined(self):

        logined = False

        if self.response and self.response.status_code == requests.codes.ok:
            if len(selector_global_container(etree.HTML(self.response.text))) == 1:
                logined = True

        return logined

    def update_user_stat_if_necessary(self):

        user_stat_ready = False
        if self.level is None or self.name is None or self.fbid is None:
            self.response = self.session.get(URL_KEEP)
            if self.response.status_code == requests.codes.ok:
                keep_page_etree = etree.HTML(self.response.text)
                keep_page_fbid = selector_keep_page_fbid(keep_page_etree)
                keep_page_name = selector_keep_page_name(keep_page_etree)

                if len(keep_page_fbid) > 0 and len(keep_page_name) > 0:
                    # fbid
                    fbid = keep_page_fbid[0].get('href').split('=')[1]
                    if fbid is not None and len(fbid) > 0:
                        self.fbid = fbid

                    # name and level
                    for n in keep_page_name:
                        te = n.text.strip()
                        ti = n.get('title')
                        if te == ti and len(ti) > 0:
                            self.name = ti
                            self.level = int(n.getparent().getnext()[0].text.split('-')[0].strip().split(' ')[1])
                            break

                if self.level is not None and self.name is not None and self.fbid is not None:
                    user_stat_ready = True
                    if self.verbose:
                        print '%s (%s) level %d' % (self.name, self.fbid, self.level)
        else:
            user_stat_ready = True

        return user_stat_ready

    def load_arena_data(self):

        loaded = False

        if self.response.status_code == requests.codes.ok:

            arena_page_etree = etree.HTML(self.response.text)
            duel_buttons = selector_arena_page_duel_buttons(arena_page_etree)
            current_token = selector_arena_page_current_token(arena_page_etree)
            stat_ref = selector_arena_page_user_stat_ref(arena_page_etree)

            # current token
            self.current_token = None
            if len(current_token):
                self.current_token = int(current_token[0].text)

            # user arena level
            self.arena_level = None
            if len(stat_ref):
                self.arena_level = arena_rank_class_mapping.index(stat_ref[-1].getparent().getparent().getnext()[0].text.strip())

            # arena battle point
            self.arena_battle_point = None
            if len(stat_ref):
                self.arena_battle_point = int(''.join(stat_ref[-1].getparent().getparent().getnext().getnext()[0].text.split(',')))

            # enemy list
            stored_duel_data = list()
            stored_timestamp = int(time.time())
            del self.enemies[:]
            if len(duel_buttons) != 20: # check if this cause crash...
                print 'duel buttons count is not 20, it is %d' % len(duel_buttons)
            for duel_button in duel_buttons:

                ref = duel_button.getparent().getparent().getprevious().getprevious().getprevious()

                if len(ref) != 3:
                    print ref
                    print 'ref length is incorrect... skip'
                    continue

                # name
                enemy_name = ref[0].text.strip()

                # level
                if ref[1].text.strip().startswith('level:'):
                    enemy_level = int(ref[1].text.strip().split(':')[1])
                else:
                    enemy_level = 1000 # not sure what level is, assume it to a high level...

                # army
                enemy_army = int(ref.getnext().text.strip())

                #duel_data
                inputs = duel_button.findall('input')
                duel_data = dict()
                for inp in inputs:
                    duel_data[inp.get('name')] = inp.get('value')

                # arena_level
                arena_lvl_img = duel_button.getparent().getparent().getprevious()[0]
                for lvl in range(1, 9):
                    trophy_name = str(lvl) + '.gif'
                    if arena_lvl_img.get('src').endswith(trophy_name):
                        arena_lvl = lvl
                        break
                else:
                    print 'Enemy arena rank level is unknown. rank: %s' % arena_lvl_img.get('src')
                    arena_lvl = 8

                self.enemies.append(ArenaEnemy(enemy_name, enemy_level, enemy_army, arena_lvl, duel_data))

                # store enemy duel data for debug purpose
                if self.save_arena_enemy_duel_data:
                    stored_duel_data.append((self.fbid, duel_data['target_id'], duel_data['bqh'], stored_timestamp))

            if self.save_arena_enemy_duel_data:
                conn = sqlite3.connect('castleage.sqlite')
                conn.executemany('INSERT INTO arena_enemy_duel_data VALUES(?, ?, ?, ?)', stored_duel_data)
                conn.commit()
                conn.close()


            # check if all arena data loaded...
            if len(self.enemies) > 0 and self.current_token is not None and self.arena_level is not None and self.arena_battle_point is not None:
                loaded = True
            elif self.verbose:
                if len(self.enemies) == 0:
                    print 'enemy list is empty.'
                if self.current_token is None:
                    print 'current token is not found.'
                if self.arena_level is None:
                    print 'arena level is not found.'
                if self.arena_battle_point is None:
                    print 'arena battle point is not found.'

        return loaded

    def choose_arena_enemy(self):

        # find out duel history data and score every enemy
        conn = sqlite3.connect('castleage.sqlite')
        c = conn.cursor()
        scores = list()
        for enemy in self.enemies:

            fbids = (self.fbid, enemy.duel_data['target_id'])
            history = c.execute('SELECT win, lose FROM arena_duel_result WHERE player_fbid = ? AND enemy_fbid = ?', fbids).fetchone()
            if history is not None:
                victory_ratio = float(history[0]) / float(history[0] + history[1])
            else:
                victory_ratio = 0.5

                         #  0,   1,   2,   3,   4,   5,   6,   7,  -7,  -6,  -5,  -4,  -3,  -2,  -1
            score_gain = [100, 105, 110, 115, 120, 125, 130, 135,  65,  70,  75,  80,  85,  90,  95][enemy.arena_level - self.arena_level] * victory_ratio
            score_lost = [-10, -10, -10,  -5,  -5,   0,   0,   0, -35, -30, -25, -20, -15, -10,  -5][enemy.arena_level - self.arena_level] * (1.0 - victory_ratio)
            score_level_diff = float(enemy.level - self.level) / float(self.level) * -20.0
            score = score_gain + score_lost + score_level_diff

            scores.append(score)

        c.close()
        conn.close()

        #print scores
        #print self.enemies

        score_enemy = sorted(zip(scores, self.enemies), reverse=True)

        for s, e in score_enemy:
            if self.target_arena_level_minus <= (e.arena_level - self.arena_level) <= self.target_arena_level_plus:

                if e.duel_data['target_id'] == '100000583069306':  # skip powerpuff
                    continue

                target = e
                break
        else:
            target = None
            print 'OMG! No target...'

        return target

    def check_arena_duel_result(self):
        '''return victory or not.
           parse response:
            - victory or defeat
            - current token
            - arena level
            - updated duel data in target (used when victory)
            - update ArenaEnemy list (used when defeat)
        '''

        victory = False
        if self.response is not None and self.response.status_code == requests.codes.ok:

            arena_page_etree = etree.HTML(self.response.text)

            result_imgs = selector_arena_page_duel_result(arena_page_etree)
            current_token = selector_arena_page_current_token(arena_page_etree)
            stat_ref = selector_arena_page_user_stat_ref(arena_page_etree)
            duel_again_buttons = selector_arena_page_duel_again_buttons(arena_page_etree)

            if len(result_imgs) and len(current_token) and len(stat_ref):
                victory = result_imgs[0].get('src').endswith('battle_victory.gif')
                self.current_token = int(current_token[0].text)
                self.arena_level = arena_rank_class_mapping.index(stat_ref[-1].getparent().getparent().getnext()[0].text.strip())

                ids = (self.fbid, self.target.duel_data['target_id']) # need to backup enemy fbid, because he may be killed and not exist in arena page
                conn = sqlite3.connect('castleage.sqlite')

                if victory: # update duel data for same target

                    if len(duel_again_buttons): # can hit again because enemy is not yet killed
                        d = dict()
                        inputs = duel_again_buttons[0].findall('input')
                        for i in inputs:
                            d[i.get('name')] = i.get('value')
                        self.target.duel_data = d

                    else: # can not hist again because enemy is killed

                        self.target = None
                        self.load_arena_data()

                    # save
                    conn.execute('INSERT OR REPLACE INTO arena_duel_result VALUES (?1, ?2, COALESCE((SELECT win+1 FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 1), COALESCE((SELECT lose FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 0))', ids)

                else:

                    conn.execute('INSERT OR REPLACE INTO arena_duel_result VALUES (?1, ?2, COALESCE((SELECT win FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 0), COALESCE((SELECT lose+1 FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 1))', ids)
                    if self.current_token > 0:
                        self.load_arena_data()

                conn.commit()
                conn.close()


        return victory


class ArenaEnemy:

    def __init__(self, name, level, army, arena_level, duel_data):
        self.name = name
        self.level = level
        self.army = army
        self.arena_level = arena_level
        self.duel_data = duel_data


class MySession(requests.Session):

    def __init__(self, store_response=False):

        super(MySession, self).__init__()
        self.store_response = store_response

    def get(self, url, **kwargs):

        r = super(MySession, self).get(url, **kwargs)
        self.store_response_if_necessary('GET', url, None, r)
        return r

    def post(self, url, data=None, json=None, **kwargs):

        r = super(MySession, self).post(url, data, json, **kwargs)
        self.store_response_if_necessary('POST', url, data, r)
        return r

    def store_response_if_necessary(self, method, url, data, response):

        if self.store_response and response is not None and response.status_code == requests.codes.ok:
            conn = sqlite3.connect('debug.sqlite')
            conn.execute('INSERT INTO response_history(response, method, url, data) VALUES (?, ?, ?, ?)', (response.text, method, url, str(data) if data is not None else ""))
            conn.commit()
            conn.close()
