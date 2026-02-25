<?php

namespace Benchmarks;

use PhpBench\Attributes\Iterations;
use PhpBench\Attributes\OutputTimeUnit;
use PhpBench\Attributes\Revs;

#[OutputTimeUnit('milliseconds', 3)]
class JsonBench
{
    private const int REVS_COUNT = 100;
    private const int ITERATIONS_COUNT = 5;
    private const int JSON_DATA_ELEMS = 5000;
    private const int START_INDEX = 0;
    private const int REPEAT_COUNT = 50;

    public function __construct(
        private string $bigJson = '',
    ) {
    }

    public function setUp(): void
    {
        $bigData = [
            'users' => []
        ];

        foreach (range(self::START_INDEX, self::JSON_DATA_ELEMS) as $i) {
            $bigData['users'][] = [
                'id' => $i,
                'name' => "User$i",
                'age' => rand(18, 80),
                'email' => "user$i@example.com",
                'tags' => ['tag1', 'tag2', 'tag3'],
            ];
        }

        $this->bigJson = json_encode($bigData, JSON_UNESCAPED_UNICODE);
    }

    #[Revs(self::REVS_COUNT)]
    #[Iterations(self::ITERATIONS_COUNT)]
    public function benchJsonRefReadOnly(): void
    {
        foreach (range(self::START_INDEX, self::REPEAT_COUNT) as $i) {
            json_get($this->bigJson, 'users.' . rand(self::START_INDEX, self::JSON_DATA_ELEMS) . '.age');
        }
    }

    #[Revs(self::REVS_COUNT)]
    #[Iterations(self::ITERATIONS_COUNT)]
    public function benchJsonEncodeDecodeReadOnly(): void
    {
        foreach (range(self::START_INDEX, self::REPEAT_COUNT) as $i) {
            $assoc = json_decode($this->bigJson, true);
            $assoc['users'][rand(self::START_INDEX, self::JSON_DATA_ELEMS)]['age'];
        }
    }

    #[Revs(self::REVS_COUNT)]
    #[Iterations(self::ITERATIONS_COUNT)]
    public function benchJsonRefChangeOnly(): void
    {
        foreach (range(self::START_INDEX, self::REPEAT_COUNT) as $i) {
            json_set($this->bigJson, 'users.' . rand(self::START_INDEX, self::JSON_DATA_ELEMS) . '.age', rand());
        }
    }

    #[Revs(self::REVS_COUNT)]
    #[Iterations(self::ITERATIONS_COUNT)]
    public function benchJsonEncodeDecodeChangeOnly(): void
    {
        $json = $this->bigJson;

        foreach (range(self::START_INDEX, self::REPEAT_COUNT) as $i) {
            $assoc = json_decode($json, true);
            $assoc['users'][rand(self::START_INDEX, self::JSON_DATA_ELEMS)]['age'] = rand();
            $json = json_encode($assoc);
        }
    }

    #[Revs(self::REVS_COUNT)]
    #[Iterations(self::ITERATIONS_COUNT)]
    public function benchJsonRefReadAndChange(): void
    {
        foreach (range(self::START_INDEX, self::REPEAT_COUNT) as $i) {
            $index = rand(self::START_INDEX, self::JSON_DATA_ELEMS);
            json_set($this->bigJson, 'users.' . $index . '.age', json_get($this->bigJson, 'users.' . $index . '.age') + 1);
        }
    }

    #[Revs(self::REVS_COUNT)]
    #[Iterations(self::ITERATIONS_COUNT)]
    public function benchJsonEncodeDecodeReadAndChange(): void
    {
        $json = $this->bigJson;

        foreach (range(self::START_INDEX, self::REPEAT_COUNT) as $i) {
            $assoc = json_decode($json, true);
            $index = rand(self::START_INDEX, self::JSON_DATA_ELEMS);
            $assoc['users'][$index]['age'] += 1;
            $json = json_encode($assoc);
        }
    }
}