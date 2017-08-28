import subprocess
from random import shuffle

class MNRandIPs:
    def __init__(self):
        self._mn_ips = []
        self._get_count = 0
        self._RESET_AT_COUNT = 20
        self._len = 0
        pass

    def refresh_mn_ips(self):
        ips = []
        result = subprocess.check_output(['sudo', 'energi', 'masternode', 'list'])
        result = result.replace('\n    ', '')
        result = result.replace('\n', '')
        result = result.replace(' : "ENABLED"', '')
        result = result.replace('"', '')
        result = result.replace('{', '').replace('}', '')
        self._mn_ips = [ ip.split(':')[0] for ip in result.split(',') ]
        shuffle( self._mn_ips )
        self._len = len( self._mn_ips )

    def get_random_x_ips(self, x):
        self._get_count += 1
        if self._get_count == self._RESET_AT_COUNT:
            self._get_count = 0
            self.refresh_mn_ips()

        for i in range(x):
            yield self._mn_ips[ (i*self._get_count) % self._len ]