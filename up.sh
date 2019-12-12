#!/usr/bin/env bash

# Please see the README for more information
docker run \
  --env IRC_SERVER='irc.example.org' \
  --env IRC_PORT='6697' \
  --env IRC_CHANNEL='#random' \
  --env IRC_NICK='watchlist-bot' \
  --env IRC_USER='watchlist-bot' \
  --env IRC_GECOS='Watchlist Bot v0.1.3 (github.com/AlexGustafsson/irc-watchlist-bot)' \
  --name irc-watchlist-bot \
  --detach \
  --restart always \
  --cpus=0.05 \
  --memory=10MB \
  axgn/irc-watchlist-bot
