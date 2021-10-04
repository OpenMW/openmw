#!/usr/bin/env python3

import click
import multiprocessing
import pathlib
import requests
import urllib.parse


@click.command()
@click.option('--token_path', type=str, default=pathlib.Path.home() / '.gitlab_token',
              help='Path to text file with Gitlab token.')
@click.option('--project_id', type=int, default=7107382,
              help='Gitlab project id.')
@click.option('--host', type=str, default='gitlab.com',
              help='Gitlab host.')
@click.option('--workers', type=int, default=10,
              help='Number of parallel workers.')
@click.option('--target_branch', type=str, default='master',
              help='Merge request target branch.')
@click.option('--begin_page', type=int, default=1,
              help='Begin with given /merge_requests page.')
@click.option('--end_page', type=int, default=4,
              help='End before given /merge_requests page.')
@click.option('--per_page', type=int, default=100,
              help='Number of merge requests per page.')
def main(token_path, project_id, host, workers, target_branch, begin_page, end_page, per_page):
    token = read_token(token_path)
    base_url = f'https://{host}/api/v4/projects/{project_id}/'
    checked = 0
    filtered = 0
    missing = 0
    for page in range(begin_page, end_page):
        merge_requests = requests.get(
            url=urllib.parse.urljoin(base_url, 'merge_requests'),
            headers={'PRIVATE-TOKEN': token},
            params=dict(state='merged', per_page=per_page, page=page),
        ).json()
        if not merge_requests:
            break
        checked += len(merge_requests)
        merge_requests = [v for v in merge_requests if v['target_branch'] == target_branch]
        if not merge_requests:
            continue
        filtered += len(merge_requests)
        with multiprocessing.Pool(workers) as pool:
            missing_merge_requests = pool.map(FilterMissingMergeRequest(token, base_url), merge_requests)
            for mr in missing_merge_requests:
                if mr is not None:
                    missing += 1
                    print(f"MR {mr['reference']} ({mr['id']}) is missing from branch {mr['target_branch']},"
                          f" previously was merged as {mr['merge_commit_sha']}")
    print(f'Checked {checked} MRs ({filtered} with {target_branch} target branch), {missing} are missing')


class FilterMissingMergeRequest:
    def __init__(self, token, base_url):
        self.token = token
        self.base_url = base_url

    def __call__(self, merge_request):
        commit_refs = requests.get(
            url=urllib.parse.urljoin(self.base_url, f"repository/commits/{merge_request['merge_commit_sha']}/refs"),
            headers={'PRIVATE-TOKEN': self.token},
        ).json()
        if 'message' in commit_refs and commit_refs['message'] == '404 Commit Not Found':
            return merge_request
        if not present_in_branch(commit_refs, branch=merge_request['target_branch']):
            return merge_request


def present_in_branch(commit_refs, branch):
    return bool(next((v for v in commit_refs if v['type'] == 'branch' and v['name'] == branch), None))


def read_token(path):
    with open(path) as stream:
        return stream.readline().strip()


if __name__ == '__main__':
    main()
