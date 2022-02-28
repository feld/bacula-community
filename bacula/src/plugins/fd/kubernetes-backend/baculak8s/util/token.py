#
#  Bacula(R) - The Network Backup Solution
#
#   Copyright (C) 2000-2022 Kern Sibbald
#
#   The original author of Bacula is Kern Sibbald, with contributions
#   from many others, a complete list can be found in the file AUTHORS.
#
#   You may use this file and others of this release according to the
#   license defined in the LICENSE file, which includes the Affero General
#   Public License, v3.0 ("AGPLv3") and some additional permissions and
#   terms pursuant to its AGPLv3 Section 7.
#
#   This notice must be preserved when any source code is
#   conveyed and/or propagated.
#
#   Bacula(R) is a registered trademark of Kern Sibbald.
#
#     Copyright (c) 2019 by Inteos sp. z o.o.
#     All rights reserved. IP transfered to Bacula Systems according to agreement.
#     Author: Radosław Korzeniewski, radekk@inteos.pl, Inteos Sp. z o.o.
#
import logging
import random
import string

TOKENSIZE = 64


def generate_token(size=TOKENSIZE, chars=tuple(string.ascii_letters) + tuple(string.digits)):
    """
        Generates a random string of characters composed on letters and digits
    :param size: the number of characters to generate - the length of the token string, default is 64
    :param chars: the allowed characters set, the default is ascii letters and digits
    :return: the token used for authorization
    """
    return ''.join(random.choice(chars) for _ in range(size))


def create_auth_data(token):
    auth_data = 'Token: {token:{size}}\n'.format(token=token, size=TOKENSIZE)
    return auth_data


def auth_data_length():
    return len(create_auth_data(''))


def check_auth_data(token, data):
    """
        Verifies the authorization data received from peer
    :param token: the token at size
    :param data:
    :return:
    """
    authdata = create_auth_data(token)
    logging.debug('AUTH_DATA:' + authdata.rstrip('\n'))
    ddata = data.decode()
    logging.debug('RECV_TOKEN_DATA:'+ddata.rstrip('\n'))
    return authdata == ddata
