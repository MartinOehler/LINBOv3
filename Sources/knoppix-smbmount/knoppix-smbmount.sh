#!/bin/bash
# knoppix-smbmount
# Display SMB shares in the local network and optionally mount them under /media
# (C) Klaus Knopper 2012
# License: GPL V2
# Requires: nbtscan, smbclient, mount.cifs


SMBTREE=""
TMP="/tmp/knoppix-smbmount.$$.tmp"
TMP2="/tmp/knoppix-smbmount.$$.tmp2"

trap bailout 3 10 15

case "$LANG" in
 de*) TITLE="SMB Netzlaufwerke"; BROWSING="Suche SMB-Server..."; SELECTHOST="Server auswählen"; SELECTSHARE="Share auswählen"; NEWSHARE="Server+Share manuell eingeben (//server/share)"; MOUNTING="Mounte %s als %s"; ALREADYMOUNTED="%s ist bereits eingebunden."; MOUNTEDAT="%s wurde erfolgreich eingebunden als %s"; MOUNTFAILED="Konnte %s nicht als %s einbinden."; CREDENTIALS="Login/Passwort eingeben für %s"; LOGIN="Benutzer (leer für Gastzugang)"; PASSWORD="Passwort"; WORKGROUP="Arbeitsgruppe (Optional)";;
 *)   TITLE="SMB Network shares"; BROWSING="Searching for SMB-Servers..."; SELECTHOST="Select Server"; SELECTSHARE="Select Share"; NEWSHARE="Enter Server+Share manually (//server/share)"; MOUNTING="Mounting %s as %s"; ALREADYMOUNTED="%s is already mounted."; MOUNTEDAT="%s successfully mounted as %s"; MOUNTFAILED="Could not mount %s as %s."; CREDENTIALS="Enter Login/Password for %s"; LOGIN="Login (empty for guest access)"; PASSWORD="Password"; WORKGROUP="Workgroup (optional)" ;;
esac

SERVERIPS=()
SERVERNAMES=()
SHARENAMES=()
DESCRIPTIONS=()
scount=0

bailout(){
 rm -f "$TMP" "$TMP2"
 exit $1
}

PROCESSPID=""
if [ -n "$DISPLAY" ]; then
 progress(){
  if [ -n "$1" ]; then
   for ((i=0; i<100; i++)); do echo "$i"; sleep 1; done 2>/dev/null | zenity --progress --width=550 --title="$TITLE" --text="$1" --auto-close --no-cancel --pulsate >/dev/null 2>&1 &
   PROCESSPID="$!"
  elif [ -n "$PROCESSPID" -a -d "/proc/$PROCESSPID" ]; then
   kill "$PROCESSPID"
  fi
 }
 selecthost(){
  local s="" server="" share="" serverip="" i
  LIST=()
  for ((i=0; i<"${#HOSTS[@]}"; i++)); do
   LIST[$((i*3))]="$((i+1))"
   LIST[$((i*3+1))]="${SERVERS[$i]}"
   LIST[$((i*3+2))]="${HOSTS[$i]}"
  done
  LIST[$((i*3))]="n"
  LIST[$((i*3+1))]="$NEWSHARE"
  LIST[$((i*3+2))]=""
  rm -f "$TMP"
  zenity --list --width=600 --height=400 --title="$TITLE" --text="$SELECTHOST" --column="" --column="Server" --column="IP" "${LIST[@]}" > "$TMP"
  read -r s < "$TMP"; rm -f "$TMP"
  [ -n "$s" ] || return 1
  [ "$s" -gt "0" ] >/dev/null 2>&1 && let s--
  echo "$s"
  return 0
 }
 selectshare(){
  local s="" server="" share="" serverip="" i
  if [ "n" = "$1" ]; then
   zenity --entry --width=550 --title="$TITLE" --text="$NEWSHARE" > "$TMP"
   read -r s <"$TMP"
   s="${s//\\//}"
   server="${s#*//}"; server="${server%%/*}"
   share="${s##*/}"
   case "$server" in
    [0-9]*) serverip="$server" ;;
         *) serverip="$(nmblookup "$server" | awk '/^[0-9]/{print $1; exit}')" ;;
   esac
   [ -n "$serverip" ] || serverip="$server"
   [ -n "$server" -a -n "$share" ] || return 1
   SERVERNAMES[$scount]="$server"
   SERVERIPS[$scount]="$serverip"
   SHARENAMES[$scount]="$share"
   let scount++
   echo "$((scount-1))"
   return 0
  else
   LIST=()
   for ((i=0; i<$scount; i++)); do
    LIST[$((i * 3))]="$((i + 1))"
    LIST[$((i * 3 + 1))]="//${SERVERNAMES[$i]}/${SHARENAMES[$i]}"
    LIST[$((i * 3 + 2))]="${DESCRIPTIONS[$i]}"
   done
   zenity --list --width=600 --height=400 --title="$TITLE" --text="$SELECTSHARE" --column="" --column="Share" --column="Description" "${LIST[@]}" > "$TMP"
   read -r s < "$TMP"
   [ -n "$s" ] || return 1
   [ "$s" -gt "0" ] >/dev/null 2>&1 && let s--
   echo "$s"; return 0
  fi
  return 1
 }
 success(){
  zenity --info --width=650 --title="$TITLE" --text="$1"
 }
 error(){
  zenity --error --width=650 --title="$TITLE" --text="$1"
  bailout 0
 }
 logindialog(){
  local server="Share"
  [ "n" = "$1" ] || server="Server ${SERVERS[$1]}"
  zenity --forms --title="$TITLE" --width=550 --separator='
' --text="$(printf "$CREDENTIALS" "$server")" --add-entry="$LOGIN" --add-password="$PASSWORD" --add-entry="$WORKGROUP" >"$TMP"
  cat "$TMP"
 }
else
 # Textmode is incomplete. :-(
 progress(){
  if [ -n "$1" ]; then
   dialog --gauge "$1" >/dev/null 2>&1 75 10
   PROCESSPID="$!"
  elif [ -n "$PROCESSPID" -a -d "/proc/$PROCESSPID" ]; then
   kill "$PROCESSPID"
  fi
 }
 success(){
  dialog --msgbox "$1" 75 10
 }
 error(){
  dialog --msgbox "$1" 75 10
  bailout 0
 }
fi

scan_hosts(){
 local host server
 progress "$BROWSING"
 HOSTS=()
 SERVERS=()
 # Get list of hosts with open SMB ports.
 LC_MESSAGES=C ip route show | awk '!/^default/{print $1}' >"$TMP"
 # DEBUG
 echo "Detected Networks:" >&2; cat "$TMP" >&2
 NETWORKS="$(<"$TMP")"
 for network in $NETWORKS; do nbtscan -m 1 -q -e "$network"; done | sort -n | uniq >"$TMP"
# LC_MESSAGES=C nmap --host-timeout 10 --open -oG "$TMP" -n -sT -p 139,445 $NETWORKS >/dev/null
# HOSTS="$(awk '/^Host:.*open/{print $2}' "$TMP")"
 hcount=0
 while read host server relax; do
  HOSTS[$hcount]="$host"
  SERVERS[$hcount]="$server"
  let hcount++
 done <"$TMP"
 progress ""
}

# scan_shares $1=serverno $2=login $3=password $4=workgroup
scan_shares(){
 local server share name description sharedesc type login="$2" password="$3" workgroup="$4"
 progress "$BROWSING"
 SERVERIPS=()
 SERVERNAMES=()
 SHARENAMES=()
 DESCRIPTIONS=()
 if [ -n "$login" -a -n "$password" ]; then
  smbclient -U "${login}%${password}" -g -L "${HOSTS[$1]}" 2>/dev/null >"$TMP"
 else
  smbclient -N -g -L "${HOSTS[$1]}" 2>/dev/null >"$TMP"
 fi
 server=""; share=""; sharedesc=""
 while IFS='|' read -r type name description; do
  case "$type" in
   [Dd][Ii][Ss][Kk])
    case "$name" in *[Pp][Rr][Ii][Nn][Tt]\$) continue;; esac # No print$ shares
    share="$name"
    sharedesc="$description"
    ;;
  esac
  if [ -n "$share" ]; then
   SERVERIPS[$scount]="${HOSTS[$1]}"
   SERVERNAMES[$scount]="${SERVERS[$1]}"
   SHARENAMES[$scount]="$share"
   DESCRIPTIONS[$scount]="$sharedesc"
   # DEBUG
   echo "Found ip=${HOSTS[$i]}, server=${SERVERS[$1]}, share=$share, description=$sharedesc" >&2
   server=""; share=""
   let scount++
  fi
 done <"$TMP"
 progress ""
}

### MAIN
scan_hosts
selecthost >"$TMP2"
read -r selected <"$TMP2"
[ -n "$selected" ] || bailout 0
# Need login/passwd now for continuing
{ read -r login; read -r password; read -r workgroup; } <<EOT
$(logindialog "$selected")
EOT
if [ -n "$login" ]; then
 CREDENTIALSFILE="/tmp/credentials.$RANDOM.txt"
 rm -f "$CREDENTIALSFILE"
 touch "$CREDENTIALSFILE"; chmod 600 "$CREDENTIALSFILE"
 cat >"$CREDENTIALSFILE" <<EOT
username=$login
password=$password
EOT
 [ -n "$workgroup" ] && echo "domain=$workgroup" >> "$CREDENTIALSFILE"
 options="users,credentials=$CREDENTIALSFILE,uid=1000,gid=1000"
else
 options="users,guest"
 [ -n "$workgroup" ] && options="$options,domain=$workgroup,uid=1000,gid=1000"
fi
[ "n" = "$selected" ] || scan_shares "$selected" "$login" "$password" "$workgroup"
selectshare "$selected" >"$TMP2"
read -r selected <"$TMP2"
[ "$selected" -ge 0 ] 2>/dev/null || bailout 0
server="${SERVERNAMES[$selected]}"
serverip="${SERVERIPS[$selected]}"
share="${SHARENAMES[$selected]}"
[ -n "$server" -a -n "$serverip" -a -n "$share" ] || bailout 0
dir="${server}_${share}"
mountpoint="/media/$dir"
[ -d "$mountpoint" ] || sudo mkdir -p "$mountpoint"
mountpoint -q "$mountpoint" && { sudo umount -l "$mountpoint"; sleep 4; }
mountpoint -q "$mountpoint" && error "$(printf "$ALREADYMOUNTED" "$mountpoint")"
# Do the mount  
progress "$(printf "$MOUNTING" "//$server/$share" "$mountpoint")"
sudo mount -t cifs -o "$options" "//$serverip/$share" "$mountpoint" 2>"$TMP" ; RC="$?"
# DEBUG
cat "$TMP" >&2
sleep 4
progress ""
# Return code may be false because of /etc/mtab, yet mount succeeds
if mountpoint -q "$mountpoint" || [ "$RC" = 0 -o "$RC" = 64 -o "$RC" = 16 ]; then
 # Add /etc/fstab entry
 ADDEDBYKNOPPIX="# Added by KNOPPIX"
 sudo sed -i -e "/^$ADDEDBYKNOPPIX/{N;/\\/\\/$serverip\\/$share /d}" /etc/fstab
 echo "$ADDEDBYKNOPPIX
//$serverip/$share $mountpoint cifs $options 0 0" | sudo tee -a /etc/fstab >/dev/null 2>&1
 # DEBUG
 echo "Created symlink $mountpoint -> $HOME/.wine/dosdevices/n:" >&2
 ln -snf "$mountpoint" "$HOME/.wine/dosdevices/n:"
 success "$(printf "$MOUNTEDAT" "//$server/$share" "$mountpoint")"
else
 error "$(printf "$MOUNTFAILED" "//$server/$share" "$mountpoint")
$(<$TMP)"
 rm -f "$CREDENTIALSFILE"
fi

bailout 0
