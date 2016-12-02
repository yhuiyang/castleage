#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sqlite3
import requests
import urllib
import pickle


AGENT = u'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.99 Safari/537.36'
URL_BASE = u'https://web3.castleagegame.com/castle_ws/'


class InvalidEmail(Exception):
    pass


class BasePlayer(object):
    def __init__(self, database_filename, email):
        self.database_connection = sqlite3.connect(database_filename)
        self.account_email = email
        self.http_get_retry_limit = 5
        self.http_post_retry_limit = 5
        self.ca_login_retry_limit = 5

        # load password from database
        cursor = self.database_connection.execute("SELECT id, password FROM accounts WHERE email = ?", (email,))
        result = cursor.fetchone()
        cursor.close()
        if result:
            self.account_id = result[0]
            self.account_password = result[1]
        else:
            raise InvalidEmail('No account with email %s.' % self.account_email)

        # create session and restore cookies if available
        self.session = requests.Session()
        self.session.headers.update({'User-Agent': AGENT})
        cursor = self.database_connection.execute("SELECT pickle FROM cookies AS c INNER JOIN accounts AS a ON c.accountId = a.id WHERE a.email = ?", (self.account_email,))
        result = cursor.fetchone()
        cursor.close()
        if result:
            self.session.cookies = pickle.loads(result[0])

    def __del__(self):
        self.database_connection.close()

    def login(self):
        login_form = {
            "platform_action": "CA_web3_login",
            "player_email": self.account_email,
            "player_password": self.account_password
        }

        for _try in range(self.ca_login_retry_limit):
            try:
                response = self.session.post(URL_BASE+u'connect_login.php', data=login_form, allow_redirects=False)
                break
            except (requests.exceptions.ConnectionError, requests.exceptions.ReadTimeout) as e:
                print 'Login retry... %d (%s)' % (_try+1, self.account_email)
                response = None
                continue
        if response is not None and response.status_code == 302:
            if 'location' in response.headers:
                if response.headers['location'] == 'index.php':
                    print self.account_email, 'Login successfully.'
                    # backup cookies for later restore
                    self.database_connection.execute('INSERT OR REPLACE INTO cookies VALUES(?, ?)', (self.account_id, pickle.dumps(self.session.cookies)))
                    self.database_connection.commit()
                    return True
                else:
                    print 'Response redirects to unexpected url:', response.headers['location']
                    return False
            else:
                print 'No "Location" header field in "302 Found" response.'
                return False
        else:
            print 'Login failed. (%s)' % (self.account_email,)
            if response is None:
                print 'response is None'
            elif response.status_code != 302:
                print 'response status code', response.status_code
            return False

    def http_get(self, php, qs=None, retry_after_login_success=True):
        if qs and len(qs) > 0:
            _url = URL_BASE + php + '?' + urllib.urlencode(qs)
        else:
            _url = URL_BASE + php
        for _try in range(self.http_get_retry_limit):
            try:
                response = self.session.get(_url, allow_redirects=False)
                break
            except (requests.exceptions.ConnectionError, requests.exceptions.ReadTimeout) as e:
                print 'GET retry... %d (%s)(%s)' % (_try+1, php, self.account_email)
                response = None
                continue
        if response is not None and response.status_code == 302:
            if 'location' in response.headers:
                # may redirect to
                #   connect_login.php or
                #   https://web4.castleagegame.com/castle_ws/connect_login.php
                if response.headers['location'] in ['connect_login.php', 'https://web4.castleagegame.com/castle_ws/connect_login.php']:
                    if self.login() and retry_after_login_success:
                        response = self.http_get(php, qs, False)
                else:
                    print 'Response redirects to unexpected url:', response.headers['location']
            else:
                print 'No "Location" header field in "302 Found" response.'
        else:
            if response is None:
                print 'Response for GET', php, 'is None'
            elif response.status_code != 200:
                print 'Response status code for GET', php, 'is', response.status_code
        return response

    def http_post(self, php, params=None, append_ajax=True, qs=None, retry_after_login_success=True):
        # construct url path
        if qs and len(qs) > 0:
            _url = URL_BASE + php + '?' + urllib.urlencode(qs)
        else:
            _url = URL_BASE + php
        # construct form post data
        form_data = [('ajax', 1)] if append_ajax else []
        if params is not None:
            if type(params) is list:
                for p in params:
                    form_data.append(p)
            elif type(params) is dict:
                for k in params.keys():
                    form_data.append((k, params[k]))
        # execute post
        for _try in range(self.http_post_retry_limit):
            try:
                response = self.session.post(_url, data=form_data, allow_redirects=False)
                break
            except (requests.exceptions.ConnectionError, requests.exceptions.ReadTimeout) as e:
                #print 'POST retry... %d (%s)(%s)(%s)' % (_try+1, php, self.account_email, form_data)  # this is too verbose
                print 'POST retry... %d (%s)(%s)' % (_try+1, php, self.account_email)
                response = None
                continue
        if response is not None and response.status_code == 302:
            if 'location' in response.headers:
                # may redirect to
                #   connect_login.php or
                #   https://web4.castleagegame.com/castle_ws/connect_login.php
                if response.headers['location'] in ['connect_login.php', 'https://web4.castleagegame.com/castle_ws/connect_login.php']:
                    if self.login() and retry_after_login_success:
                        response = self.http_post(php, params, append_ajax, qs, False)
                else:
                    print 'Response redirects to unexpected url:', response.headers['location']
            else:
                print 'No "Location" header field in "302 Found" response.'
        else:
            if response is None:
                print 'Response for POST', php, 'is None'
            elif response.status_code != 200:
                print 'Response status code for POST', php, 'is', response.status_code
        return response
