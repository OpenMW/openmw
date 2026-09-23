#!/bin/bash

set -eou pipefail
shopt -s globstar extglob

project="$(printf '%s' "${CI_PROJECT_PATH:-OpenMW/openmw}" | jq -sRr '@uri')"

if [[ -z ${GITLAB_ACCESS_TOKEN+xxxxxxx} ]]; then
    echo "This script requires GitLab API access beyond that which CI_JOB_TOKEN grants."
    echo "It must be available via an environment variable called GITLAB_ACCESS_TOKEN."
    echo
    echo "Please create a CI secret with that name containing a Personal Access Token granting the right to call the following endpoints:"
    echo "* /projects/${project}/repository/commits/:sha/statuses"
    echo "* /projects/${project}/pipelines/:pipeline_id"
    echo "* /projects/${project}/repository/commits/:sha"
    echo "* /projects/${project}/pipelines/:pipeline_id/jobs"
    echo "* /projects/${project}/jobs/:job_id/play"
    echo
    echo "At time of writing, this requires Commit Read, Pipeline Read and Job Run access."
    echo
    echo "Alternatively, if running this script locally, create such a token and just export it in your shell."
    exit 1
fi

base_commit="${CI_COMMIT_SHA:-$(git rev-parse HEAD)}"
comparison_commit="$("$(dirname -- "${BASH_SOURCE[0]}")/compute_comparison_commit.sh")"

echo "Commits: $base_commit $comparison_commit"

FILES_THAT_AFFECT_NINJA_GLOB="@(.gitlab-ci.yml|CMakeLists.txt|**/CMakeLists.txt|**/*.cmake|CI/*msvc*)"

should_trigger=""
while IFS= read -r change; do
    if [[ $change == $FILES_THAT_AFFECT_NINJA_GLOB ]]; then
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
project_url="$api/projects/$project"

if [[ -n ${CI_PIPELINE_ID:-} ]]; then
    echo "Running Ninja jobs..."
    while IFS= read -r job_id; do
        curl --no-progress-meter --request POST --header "PRIVATE-TOKEN: $access_token" --url "$project_url/jobs/$job_id/play"
    done < <(curl --no-progress-meter --header "PRIVATE-TOKEN: $access_token" --url "$project_url/pipelines/$CI_PIPELINE_ID/jobs" | jq '.[] | select(.name == "Windows_Ninja_RelWithDebInfo_GroupOne" or .name == "Windows_Ninja_RelWithDebInfo_GroupTwo") | .id')
    echo "Done."
else
    echo "Dry run not in CI, but otherwise would have run Ninja jobs."
fi
