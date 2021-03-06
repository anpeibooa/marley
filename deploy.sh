#!/bin/bash
# Script based on http://tinyurl.com/lwvvgsn

set -o errexit -o nounset

if [ "$TRAVIS_BRANCH" != "master" ]
then
  echo "This commit was made against the $TRAVIS_BRANCH and not the master! No deploy!"
  exit 0
fi

if [ "$TRAVIS_PULL_REQUEST" == "true" ]
then
  echo "This pull request won't be deployed until it is accepted and merged! No deploy!"
  exit 0
fi

rev=$(git rev-parse --short HEAD)

cd doxygen/html

git init
git config user.name "Steven Gardiner"
git config user.email "sjgardiner@ucdavis.edu"

git remote add upstream "https://$GH_TOKEN@github.com/sjgardiner/marley.git"
git fetch upstream
git reset upstream/gh-pages

echo "www.marleygen.org" > CNAME

# Creating the .nojekyll file prevents Jekyll from processing the files
# pushed to the gh-pages branch. This is needed so that some of the
# doxygen files that begin with an underscore (e.g., _event_8hh_source.html)
# will be included in the live website. See http://stackoverflow.com/a/6398875
# for more information about this problem.
touch .nojekyll

rm -f .gitignore
touch .

git add -A .
git commit -m "rebuild pages at ${rev}"
git push -q upstream HEAD:gh-pages
