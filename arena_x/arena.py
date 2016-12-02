#!/usr/bin/env python
# -*- coding: utf-8 -*-

from base_player import BasePlayer
from datetime import datetime
from lxml import html
import time, random

class colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    ENDC = '\033[0m'

ARENA_RANK_MAPPING = [  # if level < 41 (?), prepend 'Jr. ' in the rank name.
    '',
    'Brawler',
    'Swordsman',
    'Warrior',
    'Gladiator',
    'Hero',
    'Legend',
    'Alpha Vanguard',
    'Eternal Vanguard'
]

class ArenaEnemy:

    def __init__(self, name, level, army, rank, duel_params):
        self.name = name
        self.level = level
        self.army = army
        self.rank = rank
        self.duel_params = duel_params

    def __str__(self):
        return "%s lv%d army%d rank%d params%s" % (self.name, self.level, self.army, self.rank, self.duel_params)

class ArenaPlayer(BasePlayer):

    def __init__(self, email, loadout_atk=0, loadout_def=0, rankp=0, rankn=0, dbname='arena_x.db'):
        super(ArenaPlayer, self).__init__(dbname, email)

        # ca user stat
        self.fbid = None
        self.ign = None
        self.level = None

        self.loadout_atk = loadout_atk
        self.loadout_def = loadout_def
        self.rankp = rankp
        self.rankn = rankn

        # ca arena stat
        self.arena_rank = None
        self.arena_token = None
        self.arena_point = None

        # enemies
        self.target = None
        self.enemies = list()

    def __str__(self):
        return self.get_ign()

    def log(self, *args):
        print("[" + str(datetime.now()) + "][" + self.get_ign() + "] - " + " ".join(map(unicode,args)))

    def logY(self, *args):
        print("[" + str(datetime.now()) + "][" + self.get_ign() + "] - " + colors.YELLOW + " ".join(map(unicode,args)) + colors.ENDC)

    def logR(self, *args):
        print("[" + str(datetime.now()) + "][" + self.get_ign() + "] - " + colors.RED + " ".join(map(unicode,args)) + colors.ENDC)

    def logG(self, *args):
        print("[" + str(datetime.now()) + "][" + self.get_ign() + "] - " + colors.GREEN + " ".join(map(unicode,args)) + colors.ENDC)

    def switch_loadout(self, loadout):
        if loadout <= 20 and loadout >= 1:
            self.http_post('hot_swap_ajax_handler.php', params={'ajax_action':'change_loadout', 'target_loadout': loadout})

    def get_ign(self):
        # First retrieve from instance member
        if self.ign is not None:
            return self.ign

        # Second retrieve from database (cache up to 7 days)
        cursor = self.database_connection.execute('''SELECT name FROM igns WHERE accountId = ? AND datetime('now') < datetime(timestamp, '+7 days')''', (self.account_id,))
        result = cursor.fetchone()
        cursor.close()
        if result:
            self.ign = result[0]
            return self.ign

        # Final retrieve from ca server
        r = self.http_post('keep.php')
        if r and r.status_code == 200:
            doc = html.fromstring(r.text)
            names = doc.cssselect('''div[title]''')
            for name in names:
                if name.text and name.text.strip() == name.get('title'):
                    self.ign = name.get('title')
                    self.database_connection.execute('INSERT INTO igns (accountId, name) VALUES (?, ?)', (self.account_id, self.ign))
                    self.database_connection.commit()
                    return self.ign

        return self.account_email

    def get_fbid(self):

        # retrieve from instance member first
        if self.fbid is not None:
            return self.fbid

        # retrieve from database
        cursor = self.database_connection.execute('SELECT fbid FROM fbids WHERE accountId = ?', (self.account_id,))
        result = cursor.fetchone()
        cursor.close()
        if result:
            self.fbid = result[0]
            return self.fbid

        # retrieve from ca server
        r = self.http_post('keep.php')
        if r and r.status_code == 200:
            doc = html.fromstring(r.text)
            user_fbids = doc.cssselect('''div.upgrade_stats_message + div a[href^="keep.php?user="]''')
            if len(user_fbids) == 1:
                self.fbid = user_fbids[0].get('href').split('=')[-1]
                self.database_connection.execute('INSERT INTO fbids VALUES(?, ?)', (self.account_id, self.fbid))
                self.database_connection.commit()
                return self.fbid

        return self.account_email

    def get_level(self):

        # retrieve from instance member first
        if self.level is not None:
            return self.level

        # Second retrieve from database (cache up to 1 day)
        cursor = self.database_connection.execute('''SELECT lvl FROM levels WHERE accountId = ? AND datetime('now') < datetime(timestamp, '+1 day')''', (self.account_id,))
        result = cursor.fetchone()
        cursor.close()
        if result:
            self.level = result[0]
            return self.level

        # Final retrieve from ca server
        r = self.http_post('keep.php')
        if r and r.status_code == 200:
            doc = html.fromstring(r.text)
            elems = doc.cssselect('''div#main_sts_container a[href="keep.php"] > div''')
            for elem in elems:
                level_str = elem.text
                if level_str is not None and level_str.startswith('Level: '):
                    self.level = int(level_str[7:])
                    return self.level
        return -1

    def get_xp_percent(self, doc):
        percent = None
        percents = doc.cssselect('''div[style*="header_persist_xpbar.jpg"]''')
        if len(percents) == 1:
            style_attr = percents[0].get('style')
            for style in style_attr.split(';'):
                if style.startswith('width:'):
                    percent = int(style[6:-1])
                    break

        return percent

    def arena_duel(self, token_trigger=15, token_usage=15, xp_limit=None):

        r = self.http_get('arena.php')
        if r is not None and r.status_code == 200:
            doc = html.fromstring(r.text)

            self.parse_arena_page(doc)
            self.log('Init: Token[%d], Rank[%d], Points[%d]' %(self.arena_token, self.arena_rank, self.arena_point))
            #for enemy in self.enemies:
            #    self.log(enemy)
            #return

            # check if there is any xp limit
            if xp_limit is not None:
                xp_percent = self.get_xp_percent(doc)
                if xp_percent is not None and xp_percent >= xp_limit:
                    self.logR('''XP(%d) over limit(%d), skip arena duel.''' % (xp_percent, xp_limit))
                    return

            if self.arena_token > token_trigger:

                self.switch_loadout(self.loadout_atk)

                token_use = token_usage
                victory = False
                while self.arena_token > 0 and token_use > 0:
                    token_use -= 1

                    if not victory or self.target is None: # defeat or init
                        self.target = self.choose_target()
                        #self.log('Target: ', self.target)
                        #return

                    if self.target == None:
                        self.logR('No target is chosen. Is it really no suitable target? Dump enemies as below:')
                        for enemy in self.enemies:
                            self.logR(enemy)
                        break

                    # start to duel
                    #self.log(self.target.duel_params) # this check we are hitting which target...
                    r = self.http_post('arena.php', params=self.target.duel_params)
                    if r is not None and r.status_code == 200:
                        doc = html.fromstring(r.text)
                        victory, duel_again_params, messages = self.parse_arena_page(doc, True)
                        #self.log('duel again params%s' % duel_again_params)
                        #self.log('target duel params%s' % self.target.duel_params)
                        #self.log('Token[%d], Rank[%d], Points[%d]' % (self.arena_token, self.arena_rank, self.arena_point))

                        # dump extra message (bonus, warning...)
                        if messages is not None:
                            for msg in messages:
                                self.logY(msg)

                        if victory is None:
                            self.logR('vs [%s lv%d rank%d] ==> Unknown' % (self.target.name, self.target.level, self.target.rank))
                            if duel_again_params is None:
                                for msg in messages:
                                    if msg is not None and msg.startswith('Out Of Health'):
                                        # target is dead, need to change target
                                        self.logY('Confirm target is dead. Need to do something to avoid choose same target again!')
                                        break
                        else:
                            self.log('vs [%s lv%d rank%d] %s%s%s, After duel: Token[%d], Rank[%d], Point[%d]' % (self.target.name, self.target.level, self.target.rank, (colors.RED, colors.GREEN)[victory], ('Defeat', 'Victory')[victory], colors.ENDC, self.arena_token, self.arena_rank, self.arena_point))

                    else:
                        self.logR('Failed to retrieve POST response from arena.php')

                    time.sleep(float(random.randint(3000, 6000))/1000.0)

                self.switch_loadout(self.loadout_def)

        else:
            self.logR('Failed to retrieve GET response from arena.php')

    def parse_arena_page(self, doc, parse_duel_result=False):
        """
        parse:
            - current arena token
            - arena rank
            - arena battle point

        Update enemy list

        """

        #######################
        # arena user stat
        #######################
        # token
        current_tokens = doc.cssselect('div#app_body table.layout span#guild_token_current_value')
        self.arena_token = None
        if len(current_tokens) == 1:
            self.arena_token = int(current_tokens[0].text)

        # rank
        current_rank_names = doc.cssselect('div[style*="arena10_hometopbox2.jpg"] > div:nth-child(3) > div:nth-child(1)')
        self.arena_rank = None
        if len(current_rank_names):
            rank_str = current_rank_names[0].text.strip()
            if rank_str.startswith('Jr. '):  # for low level player, 'Jr. ' is prepend to their rank string.
                rank_str = rank_str[4:]
            self.arena_rank = ARENA_RANK_MAPPING.index(rank_str)

        # point
        current_battle_points = doc.cssselect('div[style*="arena10_hometopbox2.jpg"] > div:nth-child(4) > div:nth-child(1)')
        self.arena_point = None
        if len(current_battle_points):
            self.arena_point = int(''.join(current_battle_points[0].text.split(',')))  # str 1,234,567 -> int 1234567

        ######################
        # arena enemy list
        ######################
        del self.enemies[:]
        enemies = doc.cssselect('div#arena_mid div#battle_person')
        for enemy in enemies:
            enemy_name = enemy[1][0].text.strip()
            enemy_level = int(enemy[1][1].text.strip().split(':')[1])
            enemy_rank = int(enemy[1][2].text.strip().split(')')[0][-1])
            enemy_army = int(enemy[2].text)
            duel_forms = enemy.cssselect('form')
            if len(duel_forms) == 1:
                enemy_duel_params = list() # use list of tuple [(k1,v1), (k2,v2)] instead of dict {k1:v1,k2:v2}.
                inputs = duel_forms[0].findall('input')
                for inp in inputs:
                    enemy_duel_params.append((inp.get('name'), inp.get('value')))
                self.enemies.append(ArenaEnemy(enemy_name, enemy_level, enemy_army, enemy_rank, enemy_duel_params))
            #print enemy_name, enemy_level, enemy_rank, enemy_army, len(duel_forms)

        if not parse_duel_result:
             return

        #####################################
        # duel result
        # - victory, defeat, or unknown (boolean or None)
        # - duel again params (list of tuple or None)
        # - extra message (list of str, )
        #####################################
        duel_results = doc.cssselect('div.result > div.result_body > div[style*="guild_battle_result_top.jpg"] > div:nth-child(1) > div:nth-child(1)')
        if len(duel_results) == 1:
            victory = duel_results[0].text.strip() == 'VICTORY!'
        else:
            victory = None

        duel_again_buttons = doc.cssselect('''div.result > div.result_body div#arena_duel > form[onsubmit*="'arena.php'"]''')
        if len(duel_again_buttons) == 1:
            duel_again_params = list() # use list of tuple for duplicated key "action". [('action', 'guild_attack'), ('action', 'arena_battle')]
            inputs = duel_again_buttons[0].findall('input')
            for inp in inputs:
                duel_again_params.append((inp.get('name'), inp.get('value')))
        else:
            duel_again_params = None

        extra_messages = doc.cssselect('div#results_main_wrapper > div.results[style*="bg_main_middle.jpg"] > div.result > span.result_body')
        if len(extra_messages):
            messages = list()
            for msg in extra_messages:
                if msg.text is not None:
                    messages.append(msg.text.strip())
        else:
            messages = None

        # replace duel_params by duel_again_params if victory and duel_again_param exists
        if victory is True and duel_again_params is not None and len(duel_again_params) > 0:
            self.target.duel_params = duel_again_params

        # update db if we know victory result
        for k, v in self.target.duel_params:
            if k == 'target_id':
                target_id = v
        ids = (self.get_fbid(), target_id)
        if victory:
            self.database_connection.execute('INSERT OR REPLACE INTO arena_duel_result VALUES (?1, ?2, COALESCE((SELECT win+1 FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 1), COALESCE((SELECT lose FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 0))', ids)
            self.database_connection.commit()
        else:
            self.database_connection.execute('INSERT OR REPLACE INTO arena_duel_result VALUES (?1, ?2, COALESCE((SELECT win FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 0), COALESCE((SELECT lose+1 FROM arena_duel_result WHERE player_fbid=?1 AND enemy_fbid=?2), 1))', ids)
            self.database_connection.commit()

        return victory, duel_again_params, messages


    def choose_target(self):
        """
        choose target from self.enemies
        """

        scores = list()
        for enemy in self.enemies:

            target_id = None
            for k, v in enemy.duel_params: # list of tuple (k,v)
                if k == 'target_id':
                    target_id = v
            fbids = (self.get_fbid(), target_id)
            cursor = self.database_connection.execute('SELECT win, lose FROM arena_duel_result WHERE player_fbid = ? AND enemy_fbid = ?', fbids)
            history = cursor.fetchone()
            cursor.close()
            if history is not None:
                victory_ratio = float(history[0]) / float(history[0] + history[1])
            else:
                victory_ratio = 0.5

                         #  0,   1,   2,   3,   4,   5,   6,   7,  -7,  -6,  -5,  -4,  -3,  -2,  -1
            score_gain = [100, 105, 110, 115, 120, 125, 130, 135,  65,  70,  75,  80,  85,  90,  95][enemy.rank - self.arena_rank] * victory_ratio
            score_lost = [-10, -10, -10,  -5,  -5,   0,   0,   0, -35, -30, -25, -20, -15, -10,  -5][enemy.rank - self.arena_rank] * (1.0 - victory_ratio)
            score_level_diff = float(enemy.level - self.get_level()) / float(self.get_level()) * -20.0
            score = score_gain + score_lost + score_level_diff

            scores.append(score)


        #print scores
        #print self.enemies

        score_enemy = sorted(zip(scores, self.enemies), reverse=True)

        for s, e in score_enemy:
            #if self.target_arena_level_minus <= (e.rank - self.arena_rank) <= self.target_arena_level_plus:
            if self.rankn <= (e.rank - self.arena_rank) <= self.rankp:

                # if e.duel_data['target_id'] == '100000583069306':  # skip powerpuff
                #     continue
                target = e
                break
        else:
            target = None
            print 'OMG! No target...'

        return target
