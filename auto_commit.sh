clear
git st
comment="automatic save "
comment+=$(date)
git commit -am "$comment"
git push origin master
git st

