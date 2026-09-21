#!/bin/bash

set -eou pipefail
shopt -s globstar extglob

base_commit="${CI_COMMIT_SHA:-$(git rev-parse HEAD)}"
comparison_commit="$("$(dirname -- "${BASH_SOURCE[0]}")/compute_comparison_commit.sh")"

echo "Commits: $base_commit $comparison_commit"

should_trigger=""
while IFS= read -r change; do
    if [[ $change == @(.gitlab-ci.yml|CMakeLists.txt|**/CMakeLists.txt|**/*.cmake|CI/*msvc*) ]]; then
        should_trigger=1
        break
    fi
done < <(git diff --name-only $base_commit $comparison_commit)

if [[ -z $should_trigger ]]; then
    echo "Running Ninja jobs is unnecessary."
    exit 0
fi

access_token="${GITLAB_ACCESS_TOKEN:-${CI_JOB_TOKEN}}"
api="${CI_API_V4_URL:-https://gitlab.com/api/v4}"
project="$(printf '%s' "${CI_PROJECT_PATH:-OpenMW/openmw}" | jq -sRr '@uri')"
project_url="$api/projects/$project"

if [[ -n ${CI_PIPELINE_ID:-} ]]; then
    echo "Running Ninja jobs..."
    while IFS= read -r job_id; do
        curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/jobs/$job_id/play"
    done < <(curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/pipelines/$CI_PIPELINE_ID/jobs" | jq '.[] | select(.name == "Windows_Ninja_RelWithDebInfo_GroupOne" or .name == "Windows_Ninja_RelWithDebInfo_GroupTwo") | .id')
    echo "Done."
else
    echo "Dry run not in CI, but otherwise would have run Ninja jobs."
fi
