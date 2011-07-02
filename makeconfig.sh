#!/bin/bash

# Quick script to create config.h

WCREV=`git log -1 --pretty=format:%h`
WCDATE=`git log -1 --pretty=format:%cd`
WCURL=`git config --get remote.origin.url`
cat NifCommon/config.h.in | sed "s/[$]WCREV[$]/$WCREV/g" | sed "s/[$]WCDATE[$]/$WCDATE/g" | sed "s![$]WCURL[$]!$WCURL!g" > NifCommon/config.h
