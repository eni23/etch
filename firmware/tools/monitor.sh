#!/bin/bash

PIDFILE=".monitorpid"
UPLOADNOTIFY=".upload-done"

show_help() {
  echo "Usage: $0 ACTION [PORT] [BAUDRATE]"
}


if [ -z $1 ]; then
  show_help
  exit 1;
fi


if [[ $1 == "open" ]]; then
  if [ -z $2 ] || [ -z $3 ]; then
    show_help
    exit 1;
  fi
  echo $$ > "${PIDFILE}"
  screen $2 $3
  rm $PIDFILE
  exit 0
fi


if [[ $1 == "loop" ]]; then
  if [ -z $2 ] || [ -z $3 ]; then
    show_help
    exit 1;
  fi
  while true; do
    echo $$ > "${PIDFILE}"
    screen $2 $3
    rm $PIDFILE
    echo "wait for upload (press ctrl+c to exit)"
    while [ ! -f "${UPLOADNOTIFY}" ]; do
      echo -n "."
      sleep .5
    done
    rm "${UPLOADNOTIFY}"
    sleep .5
  done
  exit 0
fi


if [[ $1 == "close" ]]; then
  if [ -f "${PIDFILE}" ]; then
    SCREEN_PID=$(pgrep -P $(cat "${PIDFILE}"))
    TTY_PID=$(pgrep -P "${SCREEN_PID}")
    if [ -n "${SCREEN_PID}" ] && [ -n "${TTY_PID}" ]; then
      kill "${TTY_PID}" "${SCREEN_PID}"
    fi
  fi
  exit 0
fi


if [[ $1 == "notify-done" ]]; then
  touch "${UPLOADNOTIFY}"
  exit 0
fi


echo "Unkown action $1"
exit 1
