#!/bin/sh

# Copyright (C) 2015-2020, Wazuh Inc.

set -e
set -u

# Checking which firewall to use.
UNAME=$(uname);
FILE="default-firewall-drop.sh";

if [ "X${UNAME}" = "XFreeBSD" ]; then
    # Is ipfw enabled?
    if grep -i 'firewall_enable="YES"' /etc/rc.conf >/dev/null 2>&1; then
        # Firewall is IPFW
        FILE="ipfw.sh";
        echo "IPFW";
    fi

    # if pf enabled?
    if grep -i 'pf_enable="YES"' /etc/rc.conf >/dev/null 2>&1; then
        # Firewall is PF
        FILE="pf";
        echo "PF";
    fi

# Darwin
elif [ "X${UNAME}" = "XDarwin" ]; then
    # Is pfctl present?
    if which pfctl > /dev/null; then
        echo "PF";
        FILE="pf";
    else
        echo "IPFW";
        FILE="ipfw_mac.sh";
    fi

elif [ "X${UNAME}" = "XOpenBSD" ]; then
    if grep -i 'pf_enable="YES"' /etc/rc.conf >/dev/null 2>&1; then
        # Firewall is PF
        FILE="pf";
        echo "PF";
    fi
fi

# If file is set and execute flag is set
if [ ! "X$FILE" = "X" ]; then
    if [ $# -eq 1 ] && [ "X$1" = "Xexecute" ]; then
        cp -pr active-response/firewalls/$FILE active-response/firewall-drop.sh
    fi
fi

exit 0;
